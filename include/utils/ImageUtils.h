#ifndef IMAGECUT_IMAGEUTILS_H
#define IMAGECUT_IMAGEUTILS_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QImage>
#include <QPixmap>
#include <QString>

namespace ImageCut {
namespace Utils {

class ImageUtils {
public:
    static QImage matToQImage(const cv::Mat& mat);
    static QPixmap matToQPixmap(const cv::Mat& mat);
    static cv::Mat qImageToMat(const QImage& qimage);

    static cv::Mat loadImage(const QString& filepath);
    static bool saveImage(const QString& filepath, const cv::Mat& mat, int quality = 95);

    static cv::Mat createCheckerboardPattern(int size = 16, int squareSize = 8, uint8_t c1 = 240, uint8_t c2 = 200);
};

} // namespace Utils
} // namespace ImageCut

#endif // IMAGECUT_IMAGEUTILS_H
