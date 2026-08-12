#ifndef IMAGECUT_BASEENGINE_H
#define IMAGECUT_BASEENGINE_H

#include <opencv2/core.hpp>
#include <QString>

namespace ImageCut {
namespace AI {

class BackgroundRemovalEngine {
public:
    virtual ~BackgroundRemovalEngine() = default;

    virtual bool load(const QString& modelName = "RMBG-1.4", const QString& device = "Auto") = 0;
    virtual void unload() = 0;
    virtual bool isLoaded() const = 0;
    virtual cv::Mat process(const cv::Mat& image) = 0;
};

} // namespace AI
} // namespace ImageCut

#endif // IMAGECUT_BASEENGINE_H
