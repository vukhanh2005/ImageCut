#ifndef IMAGECUT_CROPTRANSFORM_H
#define IMAGECUT_CROPTRANSFORM_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <tuple>
#include <QRect>

namespace ImageCut {
namespace Processing {

class CropTransform {
public:
    static std::tuple<cv::Mat, cv::Mat> cropImageAndMask(
        const cv::Mat& image,
        const cv::Mat& mask,
        const cv::Rect& cropRect
    );

    static cv::Mat applyTransforms(
        const cv::Mat& image,
        double scale = 1.0,
        double rotation = 0.0,
        bool flipH = false,
        bool flipV = false
    );
};

} // namespace Processing
} // namespace ImageCut

#endif // IMAGECUT_CROPTRANSFORM_H
