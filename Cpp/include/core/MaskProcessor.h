#ifndef IMAGECUT_MASKPROCESSOR_H
#define IMAGECUT_MASKPROCESSOR_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace ImageCut {
namespace Core {

class MaskProcessor {
public:
    static cv::Mat feather(const cv::Mat& mask, double radius);
    static cv::Mat smooth(const cv::Mat& mask, int kernelSize = 5);
    static cv::Mat expandContract(const cv::Mat& mask, int value);
    static cv::Mat adjustEdgeContrast(const cv::Mat& mask, double contrast = 1.0);
    static cv::Mat decontaminateColors(const cv::Mat& rgbImage, const cv::Mat& mask, int radius = 5);
};

} // namespace Core
} // namespace ImageCut

#endif // IMAGECUT_MASKPROCESSOR_H
