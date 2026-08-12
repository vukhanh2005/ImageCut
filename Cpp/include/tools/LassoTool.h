#ifndef IMAGECUT_LASSOTOOL_H
#define IMAGECUT_LASSOTOOL_H

#include "tools/BaseTool.h"
#include <vector>
#include <QPointF>
#include <QString>

namespace ImageCut {
namespace Tools {

class LassoTool : public BaseTool {
public:
    LassoTool(UI::CanvasView* canvas, const QString& mode = "Remove");
    ~LassoTool() override = default;

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

    void drawOverlay(QPainter* painter) override;

private:
    QString m_mode = "Remove"; // "Remove" or "Keep"
    std::vector<QPointF> m_points;
    bool m_isSelecting = false;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_LASSOTOOL_H
