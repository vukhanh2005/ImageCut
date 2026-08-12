#ifndef IMAGECUT_EYEDROPPERTOOL_H
#define IMAGECUT_EYEDROPPERTOOL_H

#include "tools/BaseTool.h"
#include <QPointF>
#include <QColor>
#include <QObject>

namespace ImageCut {
namespace Tools {

class EyedropperTool : public QObject, public BaseTool {
    Q_OBJECT
public:
    explicit EyedropperTool(UI::CanvasView* canvas);
    ~EyedropperTool() override = default;

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

    void drawOverlay(QPainter* painter) override;

    QColor getPickedColor() const { return m_pickedColor; }

signals:
    void colorPickedSignal(const QColor& color);

private:
    QPointF m_currentPos;
    QColor m_pickedColor = QColor(255, 255, 255);
    bool m_hasHover = false;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_EYEDROPPERTOOL_H
