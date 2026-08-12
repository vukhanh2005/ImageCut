#include "core/Layer.h"
#include <QFont>
#include <QFontMetrics>

namespace ImageCut {
namespace Core {

Layer::Layer(const QString& name,
             const cv::Mat& img,
             const QString& type,
             double opac,
             bool vis,
             bool lock,
             const QString& blend,
             const QString& layerId)
    : name(name),
      layerType(type),
      opacity(opac),
      visible(vis),
      locked(lock),
      blendMode(blend)
{
    id = layerId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : layerId;
    if (!img.empty()) {
        image = img.clone();
    }
}

void Layer::invalidateCache() {
    dirty = true;
    cachedRgba.release();
}

int Layer::width() const {
    if (!cachedRgba.empty()) {
        return cachedRgba.cols;
    }
    if (!image.empty()) {
        return image.cols;
    }
    if (layerType == "text") {
        int margin = 30 + (textHasStroke ? textStrokeWidth * 2 : 0) + (textHasShadow ? 20 : 0);
        if (textWrapWidth > 0) {
            return std::max(60, textWrapWidth + margin * 2);
        }
        QFont font(fontFamily.isEmpty() ? "Segoe UI" : fontFamily, fontSize);
        font.setBold(fontBold);
        font.setItalic(fontItalic);
        QFontMetrics fm(font);
        int tw = fm.horizontalAdvance(textContent.isEmpty() ? "Sample Text" : textContent);
        return std::max(40, tw + margin * 2);
    }
    return 240;
}

int Layer::height() const {
    if (!cachedRgba.empty()) {
        return cachedRgba.rows;
    }
    if (!image.empty()) {
        return image.rows;
    }
    if (layerType == "text") {
        int margin = 30 + (textHasStroke ? textStrokeWidth * 2 : 0) + (textHasShadow ? 20 : 0);
        QFont font(fontFamily.isEmpty() ? "Segoe UI" : fontFamily, fontSize);
        font.setBold(fontBold);
        font.setItalic(fontItalic);
        QFontMetrics fm(font);
        QString txt = textContent.isEmpty() ? "Sample Text" : textContent;
        if (textWrapWidth > 0) {
            QRect br = fm.boundingRect(QRect(0, 0, textWrapWidth, 10000), Qt::TextWordWrap, txt);
            return std::max(30, br.height() + margin * 2);
        }
        int th = fm.height();
        return std::max(30, th + margin * 2);
    }
    return 240;
}

std::shared_ptr<Layer> Layer::clone() const {
    auto newLayer = std::make_shared<Layer>(
        name + " copy",
        image.empty() ? cv::Mat() : image.clone(),
        layerType,
        opacity,
        visible,
        locked,
        blendMode
    );

    if (!mask.empty()) {
        newLayer->mask = mask.clone();
    }

    newLayer->offsetX = offsetX;
    newLayer->offsetY = offsetY;
    newLayer->scaleX = scaleX;
    newLayer->scaleY = scaleY;
    newLayer->lockAspect = lockAspect;
    newLayer->rotation = rotation;
    newLayer->flipH = flipH;
    newLayer->flipV = flipV;

    newLayer->brightness = brightness;
    newLayer->contrast = contrast;
    newLayer->saturation = saturation;
    newLayer->exposure = exposure;
    newLayer->temperature = temperature;
    newLayer->sharpness = sharpness;

    newLayer->featherRadius = featherRadius;
    newLayer->smoothKernel = smoothKernel;
    newLayer->expandContractVal = expandContractVal;
    newLayer->edgeContrast = edgeContrast;
    newLayer->decontaminate = decontaminate;

    newLayer->parentId = parentId;
    newLayer->childrenIds = childrenIds;

    newLayer->textContent = textContent;
    newLayer->fontFamily = fontFamily;
    newLayer->fontSize = fontSize;
    newLayer->fontBold = fontBold;
    newLayer->fontItalic = fontItalic;
    newLayer->textColor = textColor;
    newLayer->textWrapWidth = textWrapWidth;
    newLayer->textAlignment = textAlignment;

    newLayer->shapeType = shapeType;
    newLayer->fillColor = fillColor;
    newLayer->strokeColor = strokeColor;
    newLayer->strokeWidth = strokeWidth;

    return newLayer;
}

} // namespace Core
} // namespace ImageCut
