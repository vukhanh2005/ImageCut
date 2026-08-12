#include "ai/ONNXEngine.h"
#include "ai/ModelManager.h"
#include "utils/Logger.h"
#include <opencv2/imgproc.hpp>
#include <QFile>

namespace ImageCut {
namespace AI {

ONNXModelEngine::ONNXModelEngine() {}

ONNXModelEngine::~ONNXModelEngine() {
    unload();
}

bool ONNXModelEngine::load(const QString& modelName, const QString& device) {
    QString modelPath = ModelManager::getInstance().getModelPath(modelName);
    if (!QFile::exists(modelPath)) {
        LOG_WARN("ONNX model file does not exist locally: " + modelPath.toStdString());
        m_isLoaded = false;
        return false;
    }

    try {
        m_net = cv::dnn::readNetFromONNX(modelPath.toStdString());
        if (m_net.empty()) {
            LOG_ERROR("Failed to parse ONNX model file: " + modelPath.toStdString());
            m_isLoaded = false;
            return false;
        }

        if (device == "CUDA" || device == "GPU") {
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }

        m_currentModelName = modelName;
        m_currentDevice = device;
        m_isLoaded = true;
        LOG_INFO("Successfully loaded ONNX model: " + modelName.toStdString());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("OpenCV DNN error loading ONNX model: ") + e.what());
        m_isLoaded = false;
        return false;
    } catch (...) {
        LOG_ERROR("Unknown error loading ONNX model.");
        m_isLoaded = false;
        return false;
    }
}

void ONNXModelEngine::unload() {
    m_net = cv::dnn::Net();
    m_currentModelName.clear();
    m_currentDevice.clear();
    m_isLoaded = false;
}

bool ONNXModelEngine::isLoaded() const {
    return m_isLoaded && !m_net.empty();
}

cv::Mat ONNXModelEngine::process(const cv::Mat& image) {
    if (image.empty()) return cv::Mat();

    cv::Mat rgbImg;
    if (image.channels() == 4) {
        cv::cvtColor(image, rgbImg, cv::COLOR_RGBA2RGB);
    } else if (image.channels() == 3) {
        rgbImg = image.clone();
    } else {
        cv::cvtColor(image, rgbImg, cv::COLOR_GRAY2RGB);
    }

    if (!m_isLoaded || m_net.empty()) {
        LOG_WARN("ONNX DNN model not loaded. Running fast classical background removal...");
        return fallbackClassicalRemove(rgbImg);
    }

    try {
        int origH = rgbImg.rows;
        int origW = rgbImg.cols;

        auto cfg = ModelManager::getInstance().getModelConfig(m_currentModelName);
        int inputW = cfg.inputSize.first > 0 ? cfg.inputSize.first : 1024;
        int inputH = cfg.inputSize.second > 0 ? cfg.inputSize.second : 1024;

        cv::Mat blob = cv::dnn::blobFromImage(
            rgbImg,
            1.0 / 255.0,
            cv::Size(inputW, inputH),
            cv::Scalar(cfg.mean[0] * 255.0, cfg.mean[1] * 255.0, cfg.mean[2] * 255.0),
            true,
            false
        );

        m_net.setInput(blob);
        cv::Mat prob = m_net.forward();

        cv::Mat predMat;
        if (prob.dims == 4) {
            predMat = cv::Mat(prob.size[2], prob.size[3], CV_32FC1, prob.ptr<float>(0, 0));
        } else {
            predMat = prob.reshape(1, prob.size[0]);
        }

        cv::Mat mask32F;
        cv::resize(predMat, mask32F, cv::Size(origW, origH), 0, 0, cv::INTER_LINEAR);

        cv::Mat mask8U;
        mask32F.convertTo(mask8U, CV_8UC1, 255.0);
        return mask8U;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("ONNX DNN inference failed: ") + e.what());
    } catch (...) {
        LOG_ERROR("Unknown error during ONNX DNN inference.");
    }

    return fallbackClassicalRemove(rgbImg);
}

cv::Mat ONNXModelEngine::fallbackClassicalRemove(const cv::Mat& rgbImg) {
    if (rgbImg.empty()) return cv::Mat();

    int origH = rgbImg.rows;
    int origW = rgbImg.cols;

    // Downscale for fast & safe segmentation (max 600px)
    double maxDim = 600.0;
    double scale = 1.0;
    if (std::max(origW, origH) > maxDim) {
        scale = maxDim / static_cast<double>(std::max(origW, origH));
    }

    int processW = std::max(20, static_cast<int>(origW * scale));
    int processH = std::max(20, static_cast<int>(origH * scale));

    cv::Mat smallRgb;
    cv::resize(rgbImg, smallRgb, cv::Size(processW, processH), 0, 0, cv::INTER_AREA);

    cv::Mat smallBgr;
    cv::cvtColor(smallRgb, smallBgr, cv::COLOR_RGB2BGR);

    cv::Mat smallMask(processH, processW, CV_8UC1, cv::Scalar(cv::GC_PR_BGD));
    cv::Mat bgdModel = cv::Mat::zeros(1, 65, CV_64FC1);
    cv::Mat fgdModel = cv::Mat::zeros(1, 65, CV_64FC1);

    int marginX = std::max(2, static_cast<int>(processW * 0.05));
    int marginY = std::max(2, static_cast<int>(processH * 0.05));
    cv::Rect rect(marginX, marginY, std::max(10, processW - 2 * marginX), std::max(10, processH - 2 * marginY));

    try {
        cv::grabCut(smallBgr, smallMask, rect, bgdModel, fgdModel, 3, cv::GC_INIT_WITH_RECT);

        cv::Mat smallBinary(processH, processW, CV_8UC1, cv::Scalar(0));
        for (int y = 0; y < processH; ++y) {
            for (int x = 0; x < processW; ++x) {
                uint8_t val = smallMask.at<uint8_t>(y, x);
                if (val == cv::GC_FGD || val == cv::GC_PR_FGD) {
                    smallBinary.at<uint8_t>(y, x) = 255;
                }
            }
        }

        // Upscale mask back to original resolution
        cv::Mat fullMask;
        cv::resize(smallBinary, fullMask, cv::Size(origW, origH), 0, 0, cv::INTER_LINEAR);
        cv::threshold(fullMask, fullMask, 127, 255, cv::THRESH_BINARY);
        return fullMask;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("GrabCut failed: ") + e.what());
    } catch (...) {
        LOG_ERROR("Unknown GrabCut exception.");
    }

    // High-quality fallback using center ellipse
    cv::Mat fallbackMask(origH, origW, CV_8UC1, cv::Scalar(0));
    cv::ellipse(fallbackMask, cv::Point(origW / 2, origH / 2), cv::Size(origW / 3, origH / 3), 0.0, 0.0, 360.0, cv::Scalar(255), -1);
    return fallbackMask;
}

} // namespace AI
} // namespace ImageCut
