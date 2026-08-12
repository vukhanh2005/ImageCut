#include "ai/ModelManager.h"
#include "utils/Logger.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace ImageCut {
namespace AI {

ModelManager& ModelManager::getInstance() {
    static ModelManager instance;
    return instance;
}

ModelManager::ModelManager() {
    QString homeDir = QDir::homePath();
    QDir dir(homeDir + "/.imagecut/models");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_modelsDir = dir.absolutePath();

    m_configs["RMBG-1.4"] = {
        "https://huggingface.co/briaai/RMBG-1.4/resolve/main/onnx/model.onnx",
        "rmbg-1.4.onnx",
        {1024, 1024},
        {0.5f, 0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f}
    };

    m_configs["U2Net"] = {
        "https://github.com/danielgatis/rembg/releases/download/v0.0.0/u2net.onnx",
        "u2net.onnx",
        {320, 320},
        {0.485f, 0.456f, 0.406f},
        {0.229f, 0.224f, 0.225f}
    };

    m_configs["Silueta"] = {
        "https://github.com/danielgatis/rembg/releases/download/v0.0.0/silueta.onnx",
        "silueta.onnx",
        {320, 320},
        {0.485f, 0.456f, 0.406f},
        {0.229f, 0.224f, 0.225f}
    };
}

QString ModelManager::getModelPath(const QString& modelName) const {
    QString name = modelName;
    if (m_configs.find(name) == m_configs.end()) {
        name = "RMBG-1.4";
    }
    return m_modelsDir + "/" + m_configs.at(name).filename;
}

bool ModelManager::isModelDownloaded(const QString& modelName) const {
    QString path = getModelPath(modelName);
    QFileInfo info(path);
    return info.exists() && info.size() > 1000;
}

QString ModelManager::downloadModel(const QString& modelName) {
    QString path = getModelPath(modelName);
    if (isModelDownloaded(modelName)) {
        LOG_INFO("Model already exists at: " + path.toStdString());
        return path;
    }
    LOG_WARN("Model file not found locally: " + path.toStdString());
    return path;
}

ModelConfig ModelManager::getModelConfig(const QString& modelName) const {
    QString name = modelName;
    if (m_configs.find(name) == m_configs.end()) {
        name = "RMBG-1.4";
    }
    return m_configs.at(name);
}

} // namespace AI
} // namespace ImageCut
