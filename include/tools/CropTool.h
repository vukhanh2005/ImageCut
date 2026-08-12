#ifndef IMAGECUT_CROPTOOL_H
#define IMAGECUT_CROPTOOL_H

#include "tools/BaseTool.h"
#include <QRectF>
#include <QString>
#include <vector>

namespace ImageCut {
namespace Tools {

class CropTool : public BaseTool {
public:
    explicit CropTool(UI::CanvasView* canvas);
    ~CropTool() override = default;

    void setAspectRatio(const QString& ratioStr);

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

    void applyCrop();
    void drawOverlay(QPainter* painter) override;

private:
    enum HandleType {
        None,
        Body,
        TopLeft, TopCenter, TopRight,
        MiddleLeft, MiddleRight,
        BottomLeft, BottomCenter, BottomRight
    };

    HandleType hitTest(const QPointF& pos) const;
    std::vector<QRectF> getHandleRects() const;

    QRectF m_cropRect;
    QString m_aspectRatio = "Free"; // "Free", "1:1 Square", "16:9 Landscape", "9:16 Portrait / Story", "4:3 Standard"
    bool m_isDragging = false;
    HandleType m_activeHandle = None;
    QPointF m_startPos;
    QRectF m_initialCropRect;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_CROPTOOL_H
