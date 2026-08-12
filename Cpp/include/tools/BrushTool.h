#ifndef IMAGECUT_BRUSHTOOL_H
#define IMAGECUT_BRUSHTOOL_H

#include "tools/BaseTool.h"
#include <opencv2/core.hpp>
#include <QString>
#include <memory>

namespace ImageCut {
namespace Core { class Layer; }
namespace Tools {

class MaskBrushTool : public BaseTool {
public:
    MaskBrushTool(UI::CanvasView* canvas, const QString& mode = "Restore");
    ~MaskBrushTool() override = default;

    void setMode(const QString& mode);

    void mousePress(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseMove(const QPointF& imgPos, QMouseEvent* event) override;
    void mouseRelease(const QPointF& imgPos, QMouseEvent* event) override;

    void drawOverlay(QPainter* painter) override;

private:
    void paintCircle(const cv::Point& center, std::shared_ptr<Core::Layer> lyr);
    void paintLine(const cv::Point& p1, const cv::Point& p2, std::shared_ptr<Core::Layer> lyr);

    QString m_mode = "Restore"; // "Restore" or "Eraser"
    int m_size = 30;            // diameter in px
    double m_hardness = 0.8;
    double m_opacity = 1.0;
    bool m_isDrawing = false;
    cv::Point m_lastPos;
    cv::Mat m_currentMask;
};

} // namespace Tools
} // namespace ImageCut

#endif // IMAGECUT_BRUSHTOOL_H
