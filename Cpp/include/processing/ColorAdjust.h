#ifndef IMAGECUT_COLORADJUST_H
#define IMAGECUT_COLORADJUST_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace ImageCut {
namespace Processing {

class ColorAdjust {
public:
    static cv::Mat applyAdjustments(
        const cv::Mat& image,
        int brightness = 0,
        int contrast = 0,
        int saturation = 0,
        int exposure = 0,
        int temperature = 0,
        int sharpness = 0
    );
};

} // namespace Processing
} // namespace ImageCut

#endif // IMAGECUT_COLORADJUST_H
