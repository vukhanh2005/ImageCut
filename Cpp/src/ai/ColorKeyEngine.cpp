#include "ai/ColorKeyEngine.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>

namespace ImageCut {
namespace AI {

ColorKeyEngine::ColorKeyEngine() {}

bool ColorKeyEngine::load(const QString& modelName, const QString& device) {
    (void)modelName; (void)device;
    return true;
}

void ColorKeyEngine::unload() {}

bool ColorKeyEngine::isLoaded() const {
    return true;
}

cv::Mat ColorKeyEngine::process(const cv::Mat& image) {
    return processColorKey(image, m_keyColor, m_tolerance, m_feather);
}

cv::Mat ColorKeyEngine::processColorKey(const cv::Mat& image, const QColor& keyColor, int tolerance, int feather) {
    if (image.empty()) return cv::Mat();

    cv::Mat rgbImg;
    if (image.channels() == 4) {
        cv::cvtColor(image, rgbImg, cv::COLOR_RGBA2RGB);
    } else {
        rgbImg = image.clone();
    }

    cv::Mat labImg;
    cv::cvtColor(rgbImg, labImg, cv::COLOR_RGB2Lab);
    labImg.convertTo(labImg, CV_32FC3);

    cv::Mat keyRgb(1, 1, CV_8UC3, cv::Scalar(keyColor.red(), keyColor.green(), keyColor.blue()));
    cv::Mat keyLab;
    cv::cvtColor(keyRgb, keyLab, cv::COLOR_RGB2Lab);
    keyLab.convertTo(keyLab, CV_32FC3);
    cv::Vec3f keyVal = keyLab.at<cv::Vec3f>(0, 0);

    int h = image.rows;
    int w = image.cols;
    cv::Mat mask(h, w, CV_8UC1, cv::Scalar(255));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            cv::Vec3f val = labImg.at<cv::Vec3f>(y, x);
            float dist = std::sqrt(
                (val[0] - keyVal[0]) * (val[0] - keyVal[0]) +
                (val[1] - keyVal[1]) * (val[1] - keyVal[1]) +
                (val[2] - keyVal[2]) * (val[2] - keyVal[2])
            );

            if (dist <= tolerance) {
                mask.at<uint8_t>(y, x) = 0;
            } else if (dist <= tolerance + feather && feather > 0) {
                float alpha = (dist - tolerance) / static_cast<float>(feather);
                mask.at<uint8_t>(y, x) = static_cast<uint8_t>(alpha * 255.0f);
            }
        }
    }

    return mask;
}

} // namespace AI
} // namespace ImageCut
