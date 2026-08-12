#include "processing/CropTransform.h"
#include <algorithm>

namespace ImageCut {
namespace Processing {

std::tuple<cv::Mat, cv::Mat> CropTransform::cropImageAndMask(
    const cv::Mat& image,
    const cv::Mat& mask,
    const cv::Rect& cropRect
) {
    if (image.empty() || cropRect.width <= 0 || cropRect.height <= 0) {
        return std::make_tuple(image, mask);
    }

    int imgW = image.cols;
    int imgH = image.rows;

    int x1 = std::max(0, std::min(cropRect.x, imgW - 1));
    int y1 = std::max(0, std::min(cropRect.y, imgH - 1));
    int x2 = std::max(x1 + 1, std::min(x1 + cropRect.width, imgW));
    int y2 = std::max(y1 + 1, std::min(y1 + cropRect.height, imgH));

    cv::Rect validRoi(x1, y1, x2 - x1, y2 - y1);
    cv::Mat croppedImg = image(validRoi).clone();
    cv::Mat croppedMask = !mask.empty() ? mask(validRoi).clone() : cv::Mat();

    return std::make_tuple(croppedImg, croppedMask);
}

cv::Mat CropTransform::applyTransforms(
    const cv::Mat& image,
    double scale,
    double rotation,
    bool flipH,
    bool flipV
) {
    if (image.empty()) return image;

    cv::Mat result = image.clone();

    if (flipH && flipV) {
        cv::flip(result, result, -1);
    } else if (flipH) {
        cv::flip(result, result, 1);
    } else if (flipV) {
        cv::flip(result, result, 0);
    }

    if (rotation != 0.0) {
        int h = result.rows;
        int w = result.cols;
        cv::Point2f center(w / 2.0f, h / 2.0f);
        cv::Mat rotMatrix = cv::getRotationMatrix2D(center, rotation, scale);
        cv::warpAffine(result, result, rotMatrix, cv::Size(w, h), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
    } else if (scale != 1.0) {
        int newW = std::max(1, static_cast<int>(result.cols * scale));
        int newH = std::max(1, static_cast<int>(result.rows * scale));
        cv::resize(result, result, cv::Size(newW, newH), 0, 0, cv::INTER_LINEAR);
    }

    return result;
}

} // namespace Processing
} // namespace ImageCut
