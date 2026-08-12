#ifndef IMAGECUT_CANVASVIEW_H
#define IMAGECUT_CANVASVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QPolygonF>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QStringList>
#include <memory>
#include <map>
#include "core/ImageDocument.h"
#include "tools/BaseTool.h"

namespace ImageCut {
namespace UI {

class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);
    ~CanvasView() override = default;

    void setDocument(std::shared_ptr<Core::ImageDocument> doc);
    std::shared_ptr<Core::ImageDocument> getDocument() const { return m_document; }

    void setActiveTool(Tools::BaseTool* tool, const QString& toolName = "Select");
    bool isSelectToolActive() const;

    void updateViewport(bool fastDrag = false);
    void fitInView();
    void setZoomLevel(double factor);
    void zoomIn();
    void zoomOut();
    void applyZoomStep(double factor);

    void panViewport(double dx, double dy);

    QPointF getHoverPos() const { return m_hoverImgPos; }

    std::shared_ptr<Core::Layer> getLayerAtPoint(const QPointF& scenePos) const;
    std::tuple<QPolygonF, QPointF, std::map<QString, QPointF>> getLayerScreenPolygon(std::shared_ptr<Core::Layer> lyr) const;
    QString getHandleAtPoint(const QPointF& scenePos) const;

signals:
    void mouseMovedSignal(const QPointF& imgPos, const QColor& pixelColor);
    void imageDroppedSignal(const QString& filePath);
    void imagesDroppedSignal(const QStringList& filePaths);
    void zoomChangedSignal(double zoomFactor);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_pixmapItem = nullptr;

    std::shared_ptr<Core::ImageDocument> m_document;
    Tools::BaseTool* m_activeTool = nullptr;
    QString m_activeToolName = "Select";
    QPointF m_hoverImgPos = QPointF(-1, -1);

    double m_zoomFactor = 1.0;
    bool m_isSpacePanning = false;
    QPointF m_panStartPos;

    // Transform drag state
    QString m_dragMode; // "", "move", "rot", "tl", "tr", etc.
    QPointF m_dragStartCanvasPos;
    std::map<QString, std::tuple<double, double, double, double, double>> m_dragStartLayerStates;

    // Smart Magnet / Snap Guide Lines
    std::vector<double> m_snapLinesX;
    std::vector<double> m_snapLinesY;
};

} // namespace UI
} // namespace ImageCut

#endif // IMAGECUT_CANVASVIEW_H
