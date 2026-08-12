#ifndef IMAGECUT_CROPTOOL_H
#define IMAGECUT_CROPTOOL_H

#include "tools/BaseTool.h"
#include <QRectF>
#include <QString>

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
    QRectF m_cropRect;
    QString m_aspectRatio = "Free"; // "Free", "1:1", "4:3", "3:4", "16:9", "9:16"
    bool m_isDragging = false;
    QPointF m_startPos;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_CROPTOOL_H
