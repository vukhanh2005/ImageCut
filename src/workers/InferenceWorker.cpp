#include "workers/InferenceWorker.h"
#include "ai/ONNXEngine.h"
#include "ai/ColorKeyEngine.h"
#include "utils/Logger.h"

namespace ImageCut {
namespace Workers {

BackgroundRemovalWorker::BackgroundRemovalWorker(
    const cv::Mat& image,
    const QString& engineType,
    const QString& modelName,
    const QString& device,
    const QColor& keyColor,
    int tolerance
) : m_image(image.clone()),
    m_engineType(engineType),
    m_modelName(modelName),
    m_device(device),
    m_keyColor(keyColor),
    m_tolerance(tolerance)
{}

void BackgroundRemovalWorker::run() {
    LOG_INFO(QString("[WorkerThread] BackgroundRemovalWorker started. Engine: %1, Model: %2, Device: %3, Image size: %4x%5")
        .arg(m_engineType).arg(m_modelName).arg(m_device).arg(m_image.cols).arg(m_image.rows).toStdString());
    try {
        emit progress(10);

        if (m_engineType == "ColorKey") {
            LOG_INFO("[WorkerThread] Running ColorKeyEngine...");
            AI::ColorKeyEngine engine;
            emit progress(50);
            cv::Mat mask = engine.processColorKey(m_image, m_keyColor, m_tolerance);
            emit progress(100);
            LOG_INFO("[WorkerThread] ColorKeyEngine complete.");
            emit resultReady(mask);
            return;
        }

        LOG_INFO("[WorkerThread] Initializing ONNXModelEngine...");
        AI::ONNXModelEngine engine;
        emit progress(25);

        if (!engine.isLoaded()) {
            LOG_INFO("[WorkerThread] Loading ONNX model: " + m_modelName.toStdString());
            engine.load(m_modelName, m_device);
        }

        emit progress(60);
        LOG_INFO("[WorkerThread] Processing image through ONNXEngine...");
        cv::Mat mask = engine.process(m_image);
        emit progress(100);

        LOG_INFO(QString("[WorkerThread] Background removal finished successfully. Mask size: %1x%2")
            .arg(mask.cols).arg(mask.rows).toStdString());
        emit resultReady(mask);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("[WorkerThread] Exception caught: ") + e.what());
        emit error(QString::fromStdString(e.what()));
    } catch (...) {
        LOG_ERROR("[WorkerThread] Unknown exception caught during execution.");
        emit error("Unknown error occurred during background removal.");
    }
}

} // namespace Workers
} // namespace ImageCut
