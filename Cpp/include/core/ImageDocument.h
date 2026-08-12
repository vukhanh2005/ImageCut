#ifndef IMAGECUT_IMAGEDOCUMENT_H
#define IMAGECUT_IMAGEDOCUMENT_H

#include <vector>
#include <memory>
#include <functional>
#include <tuple>
#include <opencv2/core.hpp>
#include <QString>
#include <QColor>
#include <QPointF>
#include "core/Layer.h"
#include "core/History.h"

namespace ImageCut {
namespace Core {

class ImageDocument {
public:
    ImageDocument(const cv::Mat& originalImage = cv::Mat(), int canvasWidth = 1920, int canvasHeight = 1080);
    ~ImageDocument() = default;

    std::shared_ptr<Layer> getActiveLayer() const;
    void setActiveLayer(std::shared_ptr<Layer> layer);
    std::vector<std::shared_ptr<Layer>> getActiveLayers() const;

    int width() const { return canvasWidth; }
    int height() const { return canvasHeight; }

    // Layer Management
    void setOriginalImage(const cv::Mat& image, const QString& layerName = "Image 1");
    std::shared_ptr<Layer> addImageLayer(const cv::Mat& image, const QString& name = "");
    std::shared_ptr<Layer> addLayer(std::shared_ptr<Layer> layer, int index = -1);
    std::shared_ptr<Layer> getLayerById(const QString& layerId) const;
    int getLayerIndex(const QString& layerId) const;

    void removeLayers(const std::vector<QString>& layerIds);
    std::vector<std::shared_ptr<Layer>> duplicateLayers(const std::vector<QString>& layerIds = {});

    // Z-Order Operations
    void reorderLayer(const QString& layerId, int newIndex);
    void moveLayerUp(const QString& layerId = "");
    void moveLayerDown(const QString& layerId = "");
    void moveLayerTop(const QString& layerId = "");
    void moveLayerBottom(const QString& layerId = "");

    // Selection
    void selectLayer(const QString& layerId, bool multiSelect = false, bool toggle = false);
    void selectAll();
    void clearSelection();

    // Grouping
    std::shared_ptr<Layer> groupLayers(const std::vector<QString>& layerIds, const QString& groupName = "Group");
    void ungroupLayer(const QString& groupId);

    // Canvas Size
    void setCanvasSize(int w, int h, const QString& anchor = "Center");

    // Coordinate Mapping Helper
    std::tuple<double, double> mapCanvasPosToLayerPos(const QPointF& canvasPos, std::shared_ptr<Layer> layer = nullptr) const;

    // Mask Updates & History
    void updateMask(const cv::Mat& newMask, const QString& layerId = "", const QString& description = "Edit Mask");

    // Observers
    void addChangeListener(std::function<void()> callback);
    void notifyChanged();

    // Document Properties
    UndoStack undoStack;

    int canvasWidth = 1920;
    int canvasHeight = 1080;

    std::vector<std::shared_ptr<Layer>> layers;
    std::vector<QString> activeLayerIds;

    // Background Configuration
    QString bgType = "Transparent"; // "Transparent", "Solid", "Image", "Gradient"
    QColor bgColor = QColor(255, 255, 255);
    QColor bgColorEnd = QColor(0, 0, 0);
    cv::Mat bgImage;
    int bgBlur = 0; // 0..50 px
    double bgOpacity = 1.0;
    double bgOffsetX = 0.0;
    double bgOffsetY = 0.0;
    double bgScale = 1.0;
    double bgRotation = 0.0;

    // View Settings
    bool showGrid = false;
    int gridSize = 20;
    double gridOpacity = 0.3;
    bool showRulers = false;
    bool showGuides = true;
    bool snapEnabled = true;

    // Mask View Modes
    QString maskViewMode = "Normal"; // "Normal", "Overlay", "BlackWhite", "Alpha"
    double maskOpacity = 0.5;

private:
    std::vector<std::function<void()>> m_changeListeners;
};

} // namespace Core
} // namespace ImageCut

#endif // IMAGECUT_IMAGEDOCUMENT_H
