#ifndef IMAGECUT_COLORKEYENGINE_H
#define IMAGECUT_COLORKEYENGINE_H

#include "ai/BaseEngine.h"
#include <QColor>

namespace ImageCut {
namespace AI {

class ColorKeyEngine : public BackgroundRemovalEngine {
public:
    ColorKeyEngine();
    ~ColorKeyEngine() override = default;

    bool load(const QString& modelName = "ColorKey", const QString& device = "Auto") override;
    void unload() override;
    bool isLoaded() const override;
    cv::Mat process(const cv::Mat& image) override;

    cv::Mat processColorKey(const cv::Mat& image, const QColor& keyColor = QColor(255, 255, 255), int tolerance = 40, int feather = 5);

private:
    QColor m_keyColor = QColor(255, 255, 255);
    int m_tolerance = 40;
    int m_feather = 5;
};

} // namespace AI
} // namespace ImageCut

#endif // IMAGECUT_COLORKEYENGINE_H
