#ifndef IMAGECUT_LAYER_H
#define IMAGECUT_LAYER_H

#include <string>
#include <vector>
#include <memory>
#include <opencv2/core.hpp>
#include <QString>
#include <QColor>
#include <QUuid>

namespace ImageCut {
namespace Core {

class Layer {
public:
    Layer(const QString& name = "Layer",
          const cv::Mat& image = cv::Mat(),
          const QString& layerType = "image",
          double opacity = 1.0,
          bool visible = true,
          bool locked = false,
          const QString& blendMode = "Normal",
          const QString& layerId = "");

    ~Layer() = default;

    std::shared_ptr<Layer> clone() const;
    void invalidateCache();

    int width() const;
    int height() const;

    // Attributes
    QString id;
    QString name;
    QString layerType; // "image", "text", "shape", "group"

    cv::Mat image; // RGB or RGBA uint8 (CV_8UC3 or CV_8UC4)
    cv::Mat mask;  // Monochrome alpha mask uint8 (CV_8UC1)

    double opacity = 1.0;
    bool visible = true;
    bool locked = false;
    QString blendMode = "Normal";

    // Transform Properties
    double offsetX = 0.0;
    double offsetY = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    bool lockAspect = true;
    double rotation = 0.0; // degrees (-360 to 360)
    bool flipH = false;
    bool flipV = false;

    // Color Adjustments
    int brightness = 0;   // -100 to 100
    int contrast = 0;     // -100 to 100
    int saturation = 0;   // -100 to 100
    int exposure = 0;     // -100 to 100
    int temperature = 0;  // -100 to 100
    int sharpness = 0;    // 0 to 100

    // Mask Post-processing Options
    double featherRadius = 0.0;
    int smoothKernel = 0;
    int expandContractVal = 0;
    double edgeContrast = 1.0;
    bool decontaminate = false;

    // Grouping & Hierarchy
    QString parentId;
    std::vector<QString> childrenIds;

    // Text Properties
    QString textContent = "Sample Text";
    QString fontFamily = "Arial";
    int fontSize = 48;
    bool fontBold = false;
    bool fontItalic = false;
    QColor textColor = QColor(255, 255, 255);

    // Rich Text Styling Properties
    bool textHasStroke = false;
    QColor textStrokeColor = QColor(0, 0, 0);
    int textStrokeWidth = 3;
    bool textHasShadow = false;
    QColor textShadowColor = QColor(0, 0, 0, 160);
    int textShadowOffsetX = 4;
    int textShadowOffsetY = 4;
    bool textHasBg = false;
    QColor textBgColor = QColor(0, 0, 0, 180);

    // Text Word Wrap & Alignment
    int textWrapWidth = 0; // 0 = Auto width, >0 = Word Wrap Width
    int textAlignment = 0; // 0 = Left, 1 = Center, 2 = Right

    // Shape Properties
    QString shapeType = "Rectangle"; // "Rectangle", "RoundedRectangle", "Circle", "Arrow", "Star", "SpeechBubble"
    QColor fillColor = QColor(0, 120, 215, 255);
    QColor strokeColor = QColor(255, 255, 255, 255);
    int strokeWidth = 2;

    // Cache & Performance Flags
    mutable cv::Mat cachedRgba;
    mutable bool dirty = true;
};

} // namespace Core
} // namespace ImageCut

#endif // IMAGECUT_LAYER_H
