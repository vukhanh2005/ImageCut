#ifndef IMAGECUT_REFINEEDGETOOL_H
#define IMAGECUT_REFINEEDGETOOL_H

#include "tools/BaseTool.h"
#include <vector>
#include <QPointF>

namespace ImageCut {
namespace Tools {

class RefineEdgeTool : public BaseTool {
public:
    explicit RefineEdgeTool(UI::CanvasView* canvas, int size = 25, int radius = 8, bool decontaminate = true);
    ~RefineEdgeTool() override = default;

    void setSize(int size) { m_size = size; }
    int getSize() const { return m_size; }

    void setRadius(int radius) { m_radius = radius; }
    int getRadius() const { return m_radius; }

    void setDecontaminate(bool enable) { m_decontaminate = enable; }
    bool isDecontaminate() const { return m_decontaminate; }

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

    void drawOverlay(QPainter* painter) override;

private:
    int m_size = 25;
    int m_radius = 8;
    bool m_decontaminate = true;

    bool m_isDrawing = false;
    std::vector<QPointF> m_points;
    QPointF m_currentPos;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_REFINEEDGETOOL_H
