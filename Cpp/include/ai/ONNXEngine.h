#ifndef IMAGECUT_ONNXENGINE_H
#define IMAGECUT_ONNXENGINE_H

#include "ai/BaseEngine.h"
#include <opencv2/dnn.hpp>
#include <memory>

namespace ImageCut {
namespace AI {

class ONNXModelEngine : public BackgroundRemovalEngine {
public:
    ONNXModelEngine();
    ~ONNXModelEngine() override;

    bool load(const QString& modelName = "RMBG-1.4", const QString& device = "Auto") override;
    void unload() override;
    bool isLoaded() const override;
    cv::Mat process(const cv::Mat& image) override;

private:
    cv::Mat fallbackClassicalRemove(const cv::Mat& rgbImg);

    cv::dnn::Net m_net;
    QString m_currentModelName;
    QString m_currentDevice;
    bool m_isLoaded = false;
};

} // namespace AI
} // namespace ImageCut

#endif // IMAGECUT_ONNXENGINE_H
