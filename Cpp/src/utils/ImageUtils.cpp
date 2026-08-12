#include "utils/ImageUtils.h"
#include "utils/Logger.h"
#include <QFile>

namespace ImageCut {
namespace Utils {

QImage ImageUtils::matToQImage(const cv::Mat& mat) {
    if (mat.empty()) {
        return QImage();
    }

    if (mat.type() == CV_8UC1) {
        QImage qimg(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
        return qimg.copy();
    } else if (mat.type() == CV_8UC3) {
        QImage qimg(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888);
        return qimg.copy();
    } else if (mat.type() == CV_8UC4) {
        QImage qimg(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGBA8888);
        return qimg.copy();
    }

    cv::Mat converted;
    if (mat.channels() == 1) {
        mat.convertTo(converted, CV_8UC1);
        QImage qimg(converted.data, converted.cols, converted.rows, static_cast<int>(converted.step), QImage::Format_Grayscale8);
        return qimg.copy();
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, converted, cv::COLOR_BGR2RGB);
        QImage qimg(converted.data, converted.cols, converted.rows, static_cast<int>(converted.step), QImage::Format_RGB888);
        return qimg.copy();
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, converted, cv::COLOR_BGRA2RGBA);
        QImage qimg(converted.data, converted.cols, converted.rows, static_cast<int>(converted.step), QImage::Format_RGBA8888);
        return qimg.copy();
    }

    return QImage();
}

QPixmap ImageUtils::matToQPixmap(const cv::Mat& mat) {
    QImage qimg = matToQImage(mat);
    return QPixmap::fromImage(qimg);
}

cv::Mat ImageUtils::qImageToMat(const QImage& qimage) {
    if (qimage.isNull()) {
        return cv::Mat();
    }

    QImage formatted = qimage.convertToFormat(QImage::Format_RGBA8888);
    cv::Mat mat(formatted.height(), formatted.width(), CV_8UC4, const_cast<uchar*>(formatted.bits()), formatted.bytesPerLine());
    return mat.clone();
}

cv::Mat ImageUtils::loadImage(const QString& filepath) {
    std::string pathStd = filepath.toStdString();
    cv::Mat img = cv::imread(pathStd, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        LOG_ERROR("Could not load image file: " + pathStd);
        return cv::Mat();
    }

    if (img.channels() == 3) {
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    } else if (img.channels() == 4) {
        cv::cvtColor(img, img, cv::COLOR_BGRA2RGBA);
    } else if (img.channels() == 1) {
        cv::cvtColor(img, img, cv::COLOR_GRAY2RGB);
    }

    return img;
}

bool ImageUtils::saveImage(const QString& filepath, const cv::Mat& mat, int quality) {
    if (mat.empty()) return false;
    std::string pathStd = filepath.toStdString();
    cv::Mat toSave = mat.clone();

    if (toSave.channels() == 3) {
        cv::cvtColor(toSave, toSave, cv::COLOR_RGB2BGR);
    } else if (toSave.channels() == 4) {
        cv::cvtColor(toSave, toSave, cv::COLOR_RGBA2BGRA);
    }

    std::vector<int> params;
    if (filepath.endsWith(".jpg", Qt::CaseInsensitive) || filepath.endsWith(".jpeg", Qt::CaseInsensitive)) {
        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(quality);
    } else if (filepath.endsWith(".webp", Qt::CaseInsensitive)) {
        params.push_back(cv::IMWRITE_WEBP_QUALITY);
        params.push_back(quality);
    } else if (filepath.endsWith(".png", Qt::CaseInsensitive)) {
        params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        params.push_back(6);
    }

    return cv::imwrite(pathStd, toSave, params);
}

cv::Mat ImageUtils::createCheckerboardPattern(int size, int squareSize, uint8_t c1, uint8_t c2) {
    cv::Mat pattern(size, size, CV_8UC3);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (((y / squareSize) + (x / squareSize)) % 2 == 0) {
                pattern.at<cv::Vec3b>(y, x) = cv::Vec3b(c1, c1, c1);
            } else {
                pattern.at<cv::Vec3b>(y, x) = cv::Vec3b(c2, c2, c2);
            }
        }
    }
    return pattern;
}

} // namespace Utils
} // namespace ImageCut
