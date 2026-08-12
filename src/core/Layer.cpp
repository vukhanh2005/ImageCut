#include "core/Layer.h"

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
    if (!image.empty()) {
        return image.cols;
    }
    return 200;
}

int Layer::height() const {
    if (!image.empty()) {
        return image.rows;
    }
    return 200;
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

    newLayer->shapeType = shapeType;
    newLayer->fillColor = fillColor;
    newLayer->strokeColor = strokeColor;
    newLayer->strokeWidth = strokeWidth;

    return newLayer;
}

} // namespace Core
} // namespace ImageCut
