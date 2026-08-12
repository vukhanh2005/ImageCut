#ifndef IMAGECUT_INFERENCEWORKER_H
#define IMAGECUT_INFERENCEWORKER_H

#include <QThread>
#include <QString>
#include <QColor>
#include <opencv2/core.hpp>

namespace ImageCut {
namespace Workers {

class BackgroundRemovalWorker : public QThread {
    Q_OBJECT
public:
    BackgroundRemovalWorker(
        const cv::Mat& image,
        const QString& engineType = "AI",
        const QString& modelName = "RMBG-1.4",
        const QString& device = "Auto",
        const QColor& keyColor = QColor(255, 255, 255),
        int tolerance = 40
    );
    ~BackgroundRemovalWorker() override = default;

signals:
    void progress(int value);
    void resultReady(const cv::Mat& mask);
    void error(const QString& errorMsg);

protected:
    void run() override;

private:
    cv::Mat m_image;
    QString m_engineType;
    QString m_modelName;
    QString m_device;
    QColor m_keyColor;
    int m_tolerance;
};

} // namespace Workers
} // namespace ImageCut

#endif // IMAGECUT_INFERENCEWORKER_H
