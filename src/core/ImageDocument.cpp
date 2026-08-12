#include "core/ImageDocument.h"
#include "utils/Logger.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ImageCut {
namespace Core {

ImageDocument::ImageDocument(const cv::Mat& originalImage, int w, int h)
    : canvasWidth(w), canvasHeight(h)
{
    if (!originalImage.empty()) {
        setOriginalImage(originalImage);
    }
}

std::shared_ptr<Layer> ImageDocument::getActiveLayer() const {
    if (!activeLayerIds.empty()) {
        auto target = getLayerById(activeLayerIds.back());
        if (target) return target;
    }
    if (!layers.empty()) {
        return layers.back();
    }
    return nullptr;
}

void ImageDocument::setActiveLayer(std::shared_ptr<Layer> layer) {
    if (layer) {
        activeLayerIds = { layer->id };
    } else {
        activeLayerIds.clear();
    }
}

std::vector<std::shared_ptr<Layer>> ImageDocument::getActiveLayers() const {
    std::vector<std::shared_ptr<Layer>> res;
    for (const auto& lid : activeLayerIds) {
        auto lyr = getLayerById(lid);
        if (lyr) res.push_back(lyr);
    }
    if (res.empty() && !layers.empty()) {
        res.push_back(layers.back());
    }
    return res;
}

void ImageDocument::setOriginalImage(const cv::Mat& image, const QString& layerName) {
    if (image.empty()) return;

    cv::Mat origRgb, initialAlpha;
    if (image.channels() == 4) {
        std::vector<cv::Mat> chs;
        cv::split(image, chs);
        initialAlpha = chs[3].clone();
        std::vector<cv::Mat> rgbChs = { chs[0], chs[1], chs[2] };
        cv::merge(rgbChs, origRgb);
    } else {
        origRgb = image.clone();
        initialAlpha = cv::Mat(image.rows, image.cols, CV_8UC1, cv::Scalar(255));
    }

    if (layers.empty()) {
        canvasWidth = image.cols;
        canvasHeight = image.rows;
    }

    auto layer = std::make_shared<Layer>(layerName, origRgb);
    layer->mask = initialAlpha;
    layer->offsetX = (canvasWidth - layer->width()) / 2.0;
    layer->offsetY = (canvasHeight - layer->height()) / 2.0;

    addLayer(layer);
    notifyChanged();
}

std::shared_ptr<Layer> ImageDocument::addImageLayer(const cv::Mat& image, const QString& name) {
    if (image.empty()) return nullptr;

    cv::Mat rgb, mask;
    if (image.channels() == 4) {
        std::vector<cv::Mat> chs;
        cv::split(image, chs);
        mask = chs[3].clone();
        std::vector<cv::Mat> rgbChs = { chs[0], chs[1], chs[2] };
        cv::merge(rgbChs, rgb);
    } else {
        rgb = image.clone();
        mask = cv::Mat(rgb.rows, rgb.cols, CV_8UC1, cv::Scalar(255));
    }

    QString layerName = name.isEmpty() ? QString("Image %1").arg(layers.size() + 1) : name;
    auto layer = std::make_shared<Layer>(layerName, rgb);
    layer->mask = mask;

    layer->offsetX = std::max(0.0, (canvasWidth - layer->width()) / 2.0);
    layer->offsetY = std::max(0.0, (canvasHeight - layer->height()) / 2.0);

    addLayer(layer);
    return layer;
}

std::shared_ptr<Layer> ImageDocument::addLayer(std::shared_ptr<Layer> layer, int index) {
    if (!layer) return nullptr;

    if (index < 0 || index >= static_cast<int>(layers.size())) {
        layers.push_back(layer);
    } else {
        layers.insert(layers.begin() + std::max(0, index), layer);
    }

    activeLayerIds = { layer->id };
    notifyChanged();
    return layer;
}

std::shared_ptr<Layer> ImageDocument::getLayerById(const QString& layerId) const {
    for (const auto& lyr : layers) {
        if (lyr->id == layerId) return lyr;
    }
    return nullptr;
}

int ImageDocument::getLayerIndex(const QString& layerId) const {
    for (size_t i = 0; i < layers.size(); ++i) {
        if (layers[i]->id == layerId) return static_cast<int>(i);
    }
    return -1;
}

void ImageDocument::removeLayers(const std::vector<QString>& layerIds) {
    if (layerIds.empty()) return;

    std::vector<std::shared_ptr<Layer>> filtered;
    for (const auto& lyr : layers) {
        if (std::find(layerIds.begin(), layerIds.end(), lyr->id) == layerIds.end()) {
            filtered.push_back(lyr);
        }
    }
    layers = filtered;

    std::vector<QString> activeFiltered;
    for (const auto& lid : activeLayerIds) {
        if (std::find(layerIds.begin(), layerIds.end(), lid) == layerIds.end()) {
            activeFiltered.push_back(lid);
        }
    }
    activeLayerIds = activeFiltered;

    if (activeLayerIds.empty() && !layers.empty()) {
        activeLayerIds = { layers.back()->id };
    }
    notifyChanged();
}

std::vector<std::shared_ptr<Layer>> ImageDocument::duplicateLayers(const std::vector<QString>& targetLayerIds) {
    std::vector<QString> targetIds = targetLayerIds.empty() ? activeLayerIds : targetLayerIds;
    std::vector<std::shared_ptr<Layer>> newLayers;

    for (const auto& lid : targetIds) {
        auto lyr = getLayerById(lid);
        if (lyr) {
            auto dup = lyr->clone();
            dup->offsetX += 20.0;
            dup->offsetY += 20.0;

            int idx = getLayerIndex(lid);
            layers.insert(layers.begin() + idx + 1, dup);
            newLayers.push_back(dup);
        }
    }

    if (!newLayers.empty()) {
        activeLayerIds.clear();
        for (const auto& l : newLayers) {
            activeLayerIds.push_back(l->id);
        }
        notifyChanged();
    }
    return newLayers;
}

void ImageDocument::reorderLayer(const QString& layerId, int newIndex) {
    int idx = getLayerIndex(layerId);
    if (idx == -1 || idx == newIndex) return;

    newIndex = std::max(0, std::min(newIndex, static_cast<int>(layers.size()) - 1));
    auto lyr = layers[idx];
    layers.erase(layers.begin() + idx);
    layers.insert(layers.begin() + newIndex, lyr);
    notifyChanged();
}

void ImageDocument::moveLayerUp(const QString& layerId) {
    QString targetId = layerId.isEmpty() ? (getActiveLayer() ? getActiveLayer()->id : "") : layerId;
    if (targetId.isEmpty()) return;

    int idx = getLayerIndex(targetId);
    if (idx != -1 && idx < static_cast<int>(layers.size()) - 1) {
        reorderLayer(targetId, idx + 1);
    }
}

void ImageDocument::moveLayerDown(const QString& layerId) {
    QString targetId = layerId.isEmpty() ? (getActiveLayer() ? getActiveLayer()->id : "") : layerId;
    if (targetId.isEmpty()) return;

    int idx = getLayerIndex(targetId);
    if (idx > 0) {
        reorderLayer(targetId, idx - 1);
    }
}

void ImageDocument::moveLayerTop(const QString& layerId) {
    QString targetId = layerId.isEmpty() ? (getActiveLayer() ? getActiveLayer()->id : "") : layerId;
    if (targetId.isEmpty()) return;

    reorderLayer(targetId, static_cast<int>(layers.size()) - 1);
}

void ImageDocument::moveLayerBottom(const QString& layerId) {
    QString targetId = layerId.isEmpty() ? (getActiveLayer() ? getActiveLayer()->id : "") : layerId;
    if (targetId.isEmpty()) return;

    reorderLayer(targetId, 0);
}

void ImageDocument::selectLayer(const QString& layerId, bool multiSelect, bool toggle) {
    if (!getLayerById(layerId)) return;

    if (multiSelect) {
        auto it = std::find(activeLayerIds.begin(), activeLayerIds.end(), layerId);
        if (toggle && it != activeLayerIds.end()) {
            if (activeLayerIds.size() > 1) {
                activeLayerIds.erase(it);
            }
        } else {
            if (it == activeLayerIds.end()) {
                activeLayerIds.push_back(layerId);
            }
        }
    } else {
        activeLayerIds = { layerId };
    }
    notifyChanged();
}

void ImageDocument::selectAll() {
    activeLayerIds.clear();
    for (const auto& l : layers) {
        activeLayerIds.push_back(l->id);
    }
    notifyChanged();
}

void ImageDocument::clearSelection() {
    activeLayerIds.clear();
    notifyChanged();
}

std::shared_ptr<Layer> ImageDocument::groupLayers(const std::vector<QString>& layerIds, const QString& groupName) {
    if (layerIds.empty()) return nullptr;

    auto groupLayer = std::make_shared<Layer>(groupName, cv::Mat(), "group");
    groupLayer->childrenIds = layerIds;

    int maxIdx = 0;
    for (const auto& lid : layerIds) {
        auto lyr = getLayerById(lid);
        if (lyr) {
            lyr->parentId = groupLayer->id;
            maxIdx = std::max(maxIdx, getLayerIndex(lid));
        }
    }

    layers.insert(layers.begin() + maxIdx + 1, groupLayer);
    activeLayerIds = { groupLayer->id };
    notifyChanged();
    return groupLayer;
}

void ImageDocument::ungroupLayer(const QString& groupId) {
    auto groupLyr = getLayerById(groupId);
    if (!groupLyr || groupLyr->layerType != "group") return;

    for (const auto& cid : groupLyr->childrenIds) {
        auto child = getLayerById(cid);
        if (child) {
            child->parentId = "";
        }
    }
    removeLayers({ groupId });
}

void ImageDocument::setCanvasSize(int w, int h, const QString& anchor) {
    if (w <= 0 || h <= 0) return;
    double dx = (w - canvasWidth) / 2.0;
    double dy = (h - canvasHeight) / 2.0;

    canvasWidth = w;
    canvasHeight = h;

    if (anchor == "Center") {
        for (auto& lyr : layers) {
            lyr->offsetX += dx;
            lyr->offsetY += dy;
        }
    }
    notifyChanged();
}

std::tuple<double, double> ImageDocument::mapCanvasPosToLayerPos(const QPointF& canvasPos, std::shared_ptr<Layer> layer) const {
    auto lyr = layer ? layer : getActiveLayer();
    if (!lyr || lyr->image.empty()) {
        return std::make_tuple(canvasPos.x(), canvasPos.y());
    }

    double canvasX = canvasPos.x();
    double canvasY = canvasPos.y();

    double layerW = static_cast<double>(lyr->width());
    double layerH = static_cast<double>(lyr->height());

    double scaleX = lyr->scaleX != 0.0 ? lyr->scaleX : 1.0;
    double scaleY = lyr->scaleY != 0.0 ? lyr->scaleY : 1.0;

    double scaledW = layerW * scaleX;
    double scaledH = layerH * scaleY;

    double dstCx = lyr->offsetX + scaledW / 2.0;
    double dstCy = lyr->offsetY + scaledH / 2.0;

    double srcCx = layerW / 2.0;
    double srcCy = layerH / 2.0;

    double dx = canvasX - dstCx;
    double dy = canvasY - dstCy;

    double rad = -(lyr->rotation * M_PI / 180.0);
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    double rx = dx * cosA - dy * sinA;
    double ry = dx * sinA + dy * cosA;

    double unscaledX = rx / scaleX;
    double unscaledY = ry / scaleY;

    double layerX = unscaledX + srcCx;
    double layerY = unscaledY + srcCy;

    if (lyr->flipH) {
        layerX = layerW - layerX;
    }
    if (lyr->flipV) {
        layerY = layerH - layerY;
    }

    return std::make_tuple(layerX, layerY);
}

void ImageDocument::updateMask(const cv::Mat& newMask, const QString& layerId, const QString& description) {
    auto target = layerId.isEmpty() ? getActiveLayer() : getLayerById(layerId);
    if (!target) return;

    target->invalidateCache();
    cv::Mat oldMask = target->mask.empty() ? cv::Mat() : target->mask.clone();

    auto cmd = std::make_unique<MaskEditCommand>(this, oldMask, newMask, target->id, description);
    undoStack.push(std::move(cmd));
}

void ImageDocument::addChangeListener(std::function<void()> callback) {
    m_changeListeners.push_back(callback);
}

void ImageDocument::notifyChanged() {
    for (auto& cb : m_changeListeners) {
        if (cb) {
            try { cb(); } catch (...) {}
        }
    }
}

} // namespace Core
} // namespace ImageCut
