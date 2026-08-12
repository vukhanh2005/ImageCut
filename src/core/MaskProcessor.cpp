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

cv::Mat MaskProcessor::guidedFilter(const cv::Mat& I_orig, const cv::Mat& p_orig, int radius, double eps) {
    if (I_orig.empty() || p_orig.empty()) return p_orig;

    cv::Mat I, p;
    if (I_orig.channels() == 4) {
        cv::cvtColor(I_orig, I, cv::COLOR_BGRA2BGR);
    } else {
        I = I_orig.clone();
    }
    I.convertTo(I, CV_32F, 1.0 / 255.0);
    p_orig.convertTo(p, CV_32F, 1.0 / 255.0);

    cv::Size ksize(radius * 2 + 1, radius * 2 + 1);

    std::vector<cv::Mat> I_ch;
    cv::split(I, I_ch);

    cv::Mat mean_p;
    cv::boxFilter(p, mean_p, CV_32F, ksize);

    cv::Mat mean_I0, mean_I1, mean_I2;
    cv::boxFilter(I_ch[0], mean_I0, CV_32F, ksize);
    cv::boxFilter(I_ch[1], mean_I1, CV_32F, ksize);
    cv::boxFilter(I_ch[2], mean_I2, CV_32F, ksize);

    cv::Mat mean_Ip0, mean_Ip1, mean_Ip2;
    cv::boxFilter(I_ch[0].mul(p), mean_Ip0, CV_32F, ksize);
    cv::boxFilter(I_ch[1].mul(p), mean_Ip1, CV_32F, ksize);
    cv::boxFilter(I_ch[2].mul(p), mean_Ip2, CV_32F, ksize);

    cv::Mat cov_Ip0 = mean_Ip0 - mean_I0.mul(mean_p);
    cv::Mat cov_Ip1 = mean_Ip1 - mean_I1.mul(mean_p);
    cv::Mat cov_Ip2 = mean_Ip2 - mean_I2.mul(mean_p);

    cv::Mat var_I0, var_I1, var_I2;
    cv::boxFilter(I_ch[0].mul(I_ch[0]), var_I0, CV_32F, ksize);
    cv::boxFilter(I_ch[1].mul(I_ch[1]), var_I1, CV_32F, ksize);
    cv::boxFilter(I_ch[2].mul(I_ch[2]), var_I2, CV_32F, ksize);
    var_I0 -= mean_I0.mul(mean_I0);
    var_I1 -= mean_I1.mul(mean_I1);
    var_I2 -= mean_I2.mul(mean_I2);

    cv::Mat a0 = cov_Ip0 / (var_I0 + eps);
    cv::Mat a1 = cov_Ip1 / (var_I1 + eps);
    cv::Mat a2 = cov_Ip2 / (var_I2 + eps);

    cv::Mat b = mean_p - (a0.mul(mean_I0) + a1.mul(mean_I1) + a2.mul(mean_I2));

    cv::Mat mean_a0, mean_a1, mean_a2, mean_b;
    cv::boxFilter(a0, mean_a0, CV_32F, ksize);
    cv::boxFilter(a1, mean_a1, CV_32F, ksize);
    cv::boxFilter(a2, mean_a2, CV_32F, ksize);
    cv::boxFilter(b, mean_b, CV_32F, ksize);

    cv::Mat q = mean_a0.mul(I_ch[0]) + mean_a1.mul(I_ch[1]) + mean_a2.mul(I_ch[2]) + mean_b;
    cv::threshold(q, q, 0.0, 0.0, cv::THRESH_TOZERO);
    q = cv::min(q, 1.0);

    cv::Mat res;
    q.convertTo(res, CV_8U, 255.0);
    return res;
}

cv::Mat MaskProcessor::refineEdgeMatting(const cv::Mat& rgbImage, const cv::Mat& existingMask, const cv::Mat& strokeRegion, int radius, double eps) {
    if (rgbImage.empty() || existingMask.empty() || strokeRegion.empty()) {
        return existingMask;
    }

    cv::Mat softAlpha = guidedFilter(rgbImage, existingMask, radius, eps);

    cv::Mat newMask = existingMask.clone();
    for (int y = 0; y < strokeRegion.rows; ++y) {
        const uint8_t* strokePtr = strokeRegion.ptr<uint8_t>(y);
        const uint8_t* alphaPtr = softAlpha.ptr<uint8_t>(y);
        uint8_t* maskPtr = newMask.ptr<uint8_t>(y);
        for (int x = 0; x < strokeRegion.cols; ++x) {
            if (strokePtr[x] > 0) {
                maskPtr[x] = alphaPtr[x];
            }
        }
    }
    return newMask;
}

} // namespace Core
} // namespace ImageCut
