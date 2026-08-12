#ifndef IMAGECUT_POLYLASSOTOOL_H
#define IMAGECUT_POLYLASSOTOOL_H

#include "tools/BaseTool.h"
#include <vector>
#include <QPointF>
#include <QString>

namespace ImageCut {
namespace Tools {

class PolyLassoTool : public BaseTool {
public:
    explicit PolyLassoTool(UI::CanvasView* canvas, const QString& mode = "Keep");
    ~PolyLassoTool() override = default;

    void setMode(const QString& mode) { m_mode = mode; }
    QString getMode() const { return m_mode; }

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;
    void keyPress(QKeyEvent* event) override;

    void drawOverlay(QPainter* painter) override;

private:
    void finishPolygon();
    void cancelPolygon();

    QString m_mode = "Keep"; // "Keep" or "Remove"
    std::vector<QPointF> m_points;
    QPointF m_currentHoverPos;
    bool m_isPlacing = false;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_POLYLASSOTOOL_H
