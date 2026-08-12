#ifndef IMAGECUT_SELECTTOOL_H
#define IMAGECUT_SELECTTOOL_H

#include "tools/BaseTool.h"

namespace ImageCut {
namespace Tools {

class SelectMoveTool : public BaseTool {
public:
    explicit SelectMoveTool(UI::CanvasView* canvas);
    ~SelectMoveTool() override = default;

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

private:
    bool m_isDragging = false;
    QPointF m_lastPos;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_SELECTTOOL_H
