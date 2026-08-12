#include "workers/ExportWorker.h"
#include "processing/Compositor.h"
#include "utils/ImageUtils.h"
#include "utils/Logger.h"

namespace ImageCut {
namespace Workers {

ExportWorker::ExportWorker(
    std::shared_ptr<Core::ImageDocument> document,
    const QString& outputPath,
    const QString& formatStr,
    int quality,
    int width,
    int height
) : m_doc(document),
    m_outputPath(outputPath),
    m_formatStr(formatStr.toUpper()),
    m_quality(quality),
    m_targetWidth(width),
    m_targetHeight(height)
{}

void ExportWorker::run() {
    try {
        if (!m_doc) {
            emit error("Null document reference.");
            return;
        }

        cv::Mat compArray = Processing::Compositor::compositeDocument(*m_doc, false);

        if (m_targetWidth > 0 && m_targetHeight > 0) {
            cv::resize(compArray, compArray, cv::Size(m_targetWidth, m_targetHeight), 0, 0, cv::INTER_LANCZOS4);
        }

        if (m_formatStr == "JPG" || m_formatStr == "JPEG") {
            cv::Mat bg(compArray.rows, compArray.cols, CV_8UC3, cv::Scalar(m_doc->bgColor.red(), m_doc->bgColor.green(), m_doc->bgColor.blue()));
            std::vector<cv::Mat> compChs;
            cv::split(compArray, compChs);

            cv::Mat alphaF;
            compChs[3].convertTo(alphaF, CV_32F, 1.0 / 255.0);

            std::vector<cv::Mat> rgbChs = { compChs[0], compChs[1], compChs[2] };
            cv::Mat fgRgb, fgF, bgF, outF, outRgb;
            cv::merge(rgbChs, fgRgb);

            fgRgb.convertTo(fgF, CV_32F);
            bg.convertTo(bgF, CV_32F);

            std::vector<cv::Mat> alpha3 = { alphaF, alphaF, alphaF };
            cv::Mat a3;
            cv::merge(alpha3, a3);

            outF = fgF.mul(a3) + bgF.mul(1.0 - a3);
            outF.convertTo(outRgb, CV_8UC3);

            Utils::ImageUtils::saveImage(m_outputPath, outRgb, m_quality);
        } else {
            Utils::ImageUtils::saveImage(m_outputPath, compArray, m_quality);
        }

        LOG_INFO("Successfully exported image to: " + m_outputPath.toStdString());
        emit finished(m_outputPath);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to export image: ") + e.what());
        emit error(QString::fromStdString(e.what()));
    }
}

} // namespace Workers
} // namespace ImageCut
