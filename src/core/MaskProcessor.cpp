#include "core/MaskProcessor.h"
#include <opencv2/photo.hpp>

namespace ImageCut {
namespace Core {

cv::Mat MaskProcessor::feather(const cv::Mat& mask, double radius) {
    if (radius <= 0.0 || mask.empty()) {
        return mask;
    }
    cv::Mat blurred;
    int ksize = static_cast<int>(radius * 4.0) | 1;
    cv::GaussianBlur(mask, blurred, cv::Size(ksize, ksize), radius, radius);
    return blurred;
}

cv::Mat MaskProcessor::smooth(const cv::Mat& mask, int kernelSize) {
    if (kernelSize < 3 || mask.empty()) {
        return mask;
    }
    if (kernelSize % 2 == 0) {
        kernelSize += 1;
    }
    cv::Mat res;
    cv::medianBlur(mask, res, kernelSize);
    return res;
}

cv::Mat MaskProcessor::expandContract(const cv::Mat& mask, int value) {
    if (value == 0 || mask.empty()) {
        return mask;
    }

    int absVal = std::abs(value);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(absVal * 2 + 1, absVal * 2 + 1));
    cv::Mat res;

    if (value > 0) {
        cv::dilate(mask, res, element);
    } else {
        cv::erode(mask, res, element);
    }
    return res;
}

cv::Mat MaskProcessor::adjustEdgeContrast(const cv::Mat& mask, double contrast) {
    if (contrast == 1.0 || mask.empty()) {
        return mask;
    }

    cv::Mat maskF;
    mask.convertTo(maskF, CV_32F, 1.0 / 255.0);
    cv::Mat centered = (maskF - 0.5) * contrast + 0.5;

    cv::Mat resultF;
    cv::threshold(centered, resultF, 0.0, 0.0, cv::THRESH_TOZERO);
    resultF = cv::min(resultF, 1.0);

    cv::Mat res;
    resultF.convertTo(res, CV_8U, 255.0);
    return res;
}

cv::Mat MaskProcessor::decontaminateColors(const cv::Mat& rgbImage, const cv::Mat& mask, int radius) {
    if (radius <= 0 || mask.empty() || rgbImage.empty()) {
        return rgbImage;
    }

    cv::Mat solidFg, edgeZone;
    cv::threshold(mask, solidFg, 200, 255, cv::THRESH_BINARY);
    
    cv::Mat inpaintMask = 255 - solidFg;

    cv::Mat cleanedRgb;
    cv::inpaint(rgbImage, inpaintMask, cleanedRgb, radius, cv::INPAINT_TELEA);

    cv::Mat alphaF;
    mask.convertTo(alphaF, CV_32FC1, 1.0 / 255.0);

    std::vector<cv::Mat> channels(3), cleanedChannels(3), outChannels(3);
    cv::split(rgbImage, channels);
    cv::split(cleanedRgb, cleanedChannels);

    for (int i = 0; i < 3; ++i) {
        cv::Mat cF, clnF, outF;
        channels[i].convertTo(cF, CV_32F);
        cleanedChannels[i].convertTo(clnF, CV_32F);

        outF = cF.mul(alphaF) + clnF.mul(1.0 - alphaF);
        outF.convertTo(outChannels[i], CV_8U);
    }

    cv::Mat result;
    cv::merge(outChannels, result);
    return result;
}

} // namespace Core
} // namespace ImageCut
