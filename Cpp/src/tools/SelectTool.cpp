#include "tools/SelectTool.h"
#include "ui/CanvasView.h"

namespace ImageCut {
namespace Tools {

SelectMoveTool::SelectMoveTool(UI::CanvasView* canvas) : BaseTool(canvas) {}

void SelectMoveTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastPos = event->position();
    }
}

void SelectMoveTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (m_isDragging && m_canvas) {
        QPointF delta = event->position() - m_lastPos;
        m_lastPos = event->position();
        m_canvas->panViewport(delta.x(), delta.y());
    }
}

void SelectMoveTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

} // namespace Tools
} // namespace ImageCut
