#ifndef IMAGECUT_MODELMANAGER_H
#define IMAGECUT_MODELMANAGER_H

#include <QString>
#include <map>
#include <vector>

namespace ImageCut {
namespace AI {

struct ModelConfig {
    QString url;
    QString filename;
    std::pair<int, int> inputSize;
    std::vector<float> mean;
    std::vector<float> std;
};

class ModelManager {
public:
    static ModelManager& getInstance();

    QString getModelPath(const QString& modelName) const;
    bool isModelDownloaded(const QString& modelName) const;
    QString downloadModel(const QString& modelName);

    ModelConfig getModelConfig(const QString& modelName) const;

private:
    ModelManager();
    ~ModelManager() = default;

    QString m_modelsDir;
    std::map<QString, ModelConfig> m_configs;
};

} // namespace AI
} // namespace ImageCut

#endif // IMAGECUT_MODELMANAGER_H
