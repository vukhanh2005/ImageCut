#include "tools/BrushTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "core/Layer.h"
#include <opencv2/imgproc.hpp>
#include <QPen>
#include <QColor>
#include <cmath>

namespace ImageCut {
namespace Tools {

MaskBrushTool::MaskBrushTool(UI::CanvasView* canvas, const QString& mode)
    : BaseTool(canvas), m_mode(mode) {}

void MaskBrushTool::setMode(const QString& mode) {
    m_mode = mode;
}

void MaskBrushTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !m_canvas || !m_canvas->getDocument()) return;

    auto doc = m_canvas->getDocument();
    auto lyr = doc->getActiveLayer();
    if (!lyr || lyr->image.empty()) return;

    m_isDrawing = true;
    auto [lx, ly] = doc->mapCanvasPosToLayerPos(imgPos, lyr);
    m_lastPos = cv::Point(static_cast<int>(std::round(lx)), static_cast<int>(std::round(ly)));

    int h = lyr->height();
    int w = lyr->width();
    if (lyr->mask.empty()) {
        lyr->mask = cv::Mat(h, w, CV_8UC1, cv::Scalar(255));
    }

    m_currentMask = lyr->mask.clone();
    paintCircle(m_lastPos, lyr);
    m_canvas->updateViewport();
}

void MaskBrushTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    if (!m_canvas) return;
    auto doc = m_canvas->getDocument();
    auto lyr = doc ? doc->getActiveLayer() : nullptr;

    if (m_isDrawing && !m_currentMask.empty() && lyr) {
        auto [lx, ly] = doc->mapCanvasPosToLayerPos(imgPos, lyr);
        cv::Point currPos(static_cast<int>(std::round(lx)), static_cast<int>(std::round(ly)));
        paintLine(m_lastPos, currPos, lyr);
        m_lastPos = currPos;
        lyr->mask = m_currentMask;
        lyr->invalidateCache();
        m_canvas->updateViewport(true);
    } else {
        if (m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void MaskBrushTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        if (m_canvas && m_canvas->getDocument()) {
            auto doc = m_canvas->getDocument();
            auto lyr = doc->getActiveLayer();
            if (lyr && !m_currentMask.empty()) {
                QString desc = (m_mode == "Restore") ? "Restore Brush" : "Erase Brush";
                doc->updateMask(m_currentMask, lyr->id, desc);
            }
        }
        m_currentMask.release();
        m_canvas->updateViewport();
    }
}

void MaskBrushTool::paintCircle(const cv::Point& center, std::shared_ptr<Core::Layer> lyr) {
    if (m_currentMask.empty() || !lyr) return;

    double scale = std::max(0.1, std::max(lyr->scaleX, lyr->scaleY));
    int r = std::max(1, static_cast<int>(std::round((m_size / 2.0) / scale)));
    uint8_t val = (m_mode == "Restore") ? 255 : 0;

    cv::circle(m_currentMask, center, r, cv::Scalar(val), -1);
}

void MaskBrushTool::paintLine(const cv::Point& p1, const cv::Point& p2, std::shared_ptr<Core::Layer> lyr) {
    if (m_currentMask.empty() || !lyr) return;

    double scale = std::max(0.1, std::max(lyr->scaleX, lyr->scaleY));
    int r = std::max(1, static_cast<int>(std::round((m_size / 2.0) / scale)));
    uint8_t val = (m_mode == "Restore") ? 255 : 0;

    cv::line(m_currentMask, p1, p2, cv::Scalar(val), r * 2);
}

void MaskBrushTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter) return;
    QPointF hoverPos = m_canvas->getHoverPos();
    if (hoverPos.x() >= 0 && hoverPos.y() >= 0) {
        painter->setRenderHint(QPainter::Antialiasing);
        QColor penColor = (m_mode == "Restore") ? QColor(0, 255, 128) : QColor(255, 64, 64);
        QPen pen(penColor, 1.5, Qt::SolidLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        double r = m_size / 2.0;
        painter->drawEllipse(hoverPos, r, r);
    }
}

} // namespace Tools
} // namespace ImageCut
