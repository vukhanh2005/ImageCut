#include "workers/BatchWorker.h"
#include "ai/ONNXEngine.h"
#include "utils/ImageUtils.h"
#include "utils/Logger.h"
#include <QDir>
#include <QFileInfo>

namespace ImageCut {
namespace Workers {

BatchWorker::BatchWorker(
    const QStringList& filePaths,
    const QString& outputDir,
    const QString& modelName,
    const QString& outputFormat,
    int quality
) : m_filePaths(filePaths),
    m_outputDir(outputDir),
    m_modelName(modelName),
    m_outputFormat(outputFormat.toUpper()),
    m_quality(quality)
{}

void BatchWorker::cancel() {
    m_isCancelled = true;
}

void BatchWorker::run() {
    QDir dir(m_outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    AI::ONNXModelEngine engine;
    engine.load(m_modelName);

    int successCount = 0;
    int failCount = 0;
    int total = m_filePaths.size();

    for (int idx = 0; idx < total; ++idx) {
        if (m_isCancelled) break;

        QString filePath = m_filePaths[idx];
        QFileInfo info(filePath);
        QString filename = info.fileName();
        emit progress(idx + 1, total, filename);

        try {
            cv::Mat rgbImg = Utils::ImageUtils::loadImage(filePath);
            if (rgbImg.empty()) {
                failCount++;
                continue;
            }

            cv::Mat mask = engine.process(rgbImg);

            QString baseName = info.completeBaseName();
            QString ext = (m_outputFormat == "PNG") ? ".png" : ("." + m_outputFormat.toLower());
            QString outPath = m_outputDir + "/" + baseName + "_nobg" + ext;

            if (m_outputFormat == "PNG") {
                std::vector<cv::Mat> chs;
                cv::split(rgbImg, chs);
                if (chs.size() == 3) {
                    chs.push_back(mask);
                } else if (chs.size() == 4) {
                    chs[3] = mask;
                }
                cv::Mat rgba;
                cv::merge(chs, rgba);
                Utils::ImageUtils::saveImage(outPath, rgba, m_quality);
            } else if (m_outputFormat == "JPG" || m_outputFormat == "JPEG") {
                cv::Mat bg(rgbImg.rows, rgbImg.cols, CV_8UC3, cv::Scalar(255, 255, 255));
                cv::Mat alphaF, rgbF, bgF, compF;
                mask.convertTo(alphaF, CV_32FC1, 1.0 / 255.0);
                rgbImg.convertTo(rgbF, CV_32FC3);
                bg.convertTo(bgF, CV_32FC3);

                std::vector<cv::Mat> alphaChs = { alphaF, alphaF, alphaF };
                cv::Mat alpha3;
                cv::merge(alphaChs, alpha3);

                compF = rgbF.mul(alpha3) + bgF.mul(1.0 - alpha3);
                cv::Mat comp;
                compF.convertTo(comp, CV_8UC3);
                Utils::ImageUtils::saveImage(outPath, comp, m_quality);
            } else {
                std::vector<cv::Mat> chs;
                cv::split(rgbImg, chs);
                if (chs.size() == 3) chs.push_back(mask);
                cv::Mat rgba;
                cv::merge(chs, rgba);
                Utils::ImageUtils::saveImage(outPath, rgba, m_quality);
            }

            successCount++;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to batch process: " + filePath.toStdString() + " - " + e.what());
            failCount++;
        }
    }

    emit finished(successCount, failCount);
}

} // namespace Workers
} // namespace ImageCut
