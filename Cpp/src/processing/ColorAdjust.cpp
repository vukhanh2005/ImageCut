#include "processing/ColorAdjust.h"
#include <algorithm>

namespace ImageCut {
namespace Processing {

cv::Mat ColorAdjust::applyAdjustments(
    const cv::Mat& image,
    int brightness,
    int contrast,
    int saturation,
    int exposure,
    int temperature,
    int sharpness
) {
    if (image.empty()) return image;

    cv::Mat imgF;
    image.convertTo(imgF, CV_32F);

    // 1. Brightness & Exposure (-100 to 100)
    if (brightness != 0 || exposure != 0) {
        double totalB = brightness * 1.25 + exposure * 1.5;
        imgF += totalB;
    }

    // 2. Contrast (-100 to 100)
    if (contrast != 0) {
        double factor = (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast));
        imgF = factor * (imgF - 128.0) + 128.0;
    }

    cv::threshold(imgF, imgF, 0.0, 0.0, cv::THRESH_TOZERO);
    imgF = cv::min(imgF, 255.0);

    // 3. Saturation (-100 to 100)
    if (saturation != 0) {
        cv::Mat img8;
        imgF.convertTo(img8, CV_8U);
        cv::Mat hsv;
        cv::cvtColor(img8, hsv, cv::COLOR_RGB2HSV);
        hsv.convertTo(hsv, CV_32F);

        std::vector<cv::Mat> hsvChannels;
        cv::split(hsv, hsvChannels);

        double satScale = 1.0 + (saturation / 100.0);
        hsvChannels[1] = cv::min(hsvChannels[1] * satScale, 255.0);

        cv::merge(hsvChannels, hsv);
        hsv.convertTo(hsv, CV_8U);
        cv::cvtColor(hsv, img8, cv::COLOR_HSV2RGB);
        img8.convertTo(imgF, CV_32F);
    }

    // 4. Color Temperature (-100 to 100)
    if (temperature != 0) {
        double tempVal = temperature * 0.5;
        std::vector<cv::Mat> rgbChannels;
        cv::split(imgF, rgbChannels);

        rgbChannels[0] = cv::min(cv::max(rgbChannels[0] + tempVal, 0.0), 255.0); // Red
        rgbChannels[2] = cv::min(cv::max(rgbChannels[2] - tempVal, 0.0), 255.0); // Blue

        cv::merge(rgbChannels, imgF);
    }

    cv::Mat result;
    imgF.convertTo(result, CV_8U);

    // 5. Sharpness (0 to 100)
    if (sharpness > 0) {
        double kernelAmount = (sharpness / 100.0) * 1.5;
        cv::Mat blurred;
        cv::GaussianBlur(result, blurred, cv::Size(0, 0), 3.0);
        cv::addWeighted(result, 1.0 + kernelAmount, blurred, -kernelAmount, 0.0, result);
    }

    return result;
}

} // namespace Processing
} // namespace ImageCut
