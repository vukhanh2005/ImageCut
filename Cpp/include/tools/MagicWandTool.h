#ifndef IMAGECUT_MAGICWANDTOOL_H
#define IMAGECUT_MAGICWANDTOOL_H

#include "tools/BaseTool.h"

namespace ImageCut {
namespace Tools {

class MagicWandTool : public BaseTool {
public:
    explicit MagicWandTool(UI::CanvasView* canvas);
    ~MagicWandTool() override = default;

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

    void drawOverlay(QPainter* painter) override;

private:
    int m_tolerance = 30; // 1 to 100
    bool m_contiguous = true;
    int m_feather = 2;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_MAGICWANDTOOL_H
