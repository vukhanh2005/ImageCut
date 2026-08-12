#ifndef IMAGECUT_BASETOOL_H
#define IMAGECUT_BASETOOL_H

#include <QPointF>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>

namespace ImageCut {
namespace UI { class CanvasView; }
namespace Tools {

class BaseTool {
public:
    explicit BaseTool(UI::CanvasView* canvas) : m_canvas(canvas) {}
    virtual ~BaseTool() = default;

    virtual void mousePress(const QPointF& imgPos, QMouseEvent* event) = 0;
    virtual void mouseMove(const QPointF& imgPos, QMouseEvent* event) = 0;
    virtual void mouseRelease(const QPointF& imgPos, QMouseEvent* event) = 0;

    virtual void drawOverlay(QPainter* painter) { (void)painter; }
    virtual void keyPress(QKeyEvent* event) { (void)event; }

protected:
    UI::CanvasView* m_canvas;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_BASETOOL_H
