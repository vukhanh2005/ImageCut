#include "tools/RefineEdgeTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "core/Layer.h"
#include "core/MaskProcessor.h"
#include "utils/Logger.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace ImageCut {
namespace Tools {

RefineEdgeTool::RefineEdgeTool(UI::CanvasView* canvas, int size, int radius, bool decontaminate)
    : BaseTool(canvas), m_size(size), m_radius(radius), m_decontaminate(decontaminate)
{}

void RefineEdgeTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    m_isDrawing = true;
    m_points.clear();
    m_points.push_back(imgPos);
    m_currentPos = imgPos;
    if (m_canvas && m_canvas->viewport()) {
        m_canvas->viewport()->update();
    }
}

void RefineEdgeTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    m_currentPos = imgPos;
    if (m_isDrawing) {
        m_points.push_back(imgPos);
    }
    if (m_canvas && m_canvas->viewport()) {
        m_canvas->viewport()->update();
    }
}

void RefineEdgeTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() != Qt::LeftButton || !m_isDrawing) return;
    m_isDrawing = false;

    if (m_points.empty() || !m_canvas || !m_canvas->getDocument()) {
        m_points.clear();
        return;
    }

    auto doc = m_canvas->getDocument();
    auto lyr = doc->getActiveLayer();

    if (lyr && !lyr->locked && !lyr->image.empty()) {
        int w = lyr->width();
        int h = lyr->height();

        // Create initial mask if layer doesn't have one
        cv::Mat curMask = lyr->mask.empty() ? cv::Mat(h, w, CV_8UC1, cv::Scalar(255)) : lyr->mask.clone();

        // Generate stroke region mask
        cv::Mat strokeMask(h, w, CV_8UC1, cv::Scalar(0));
        int r = std::max(1, m_size / 2);

        for (size_t i = 0; i < m_points.size(); ++i) {
            auto [lx, ly] = doc->mapCanvasPosToLayerPos(m_points[i], lyr);
            int ix = std::clamp(static_cast<int>(std::round(lx)), 0, w - 1);
            int iy = std::clamp(static_cast<int>(std::round(ly)), 0, h - 1);

            if (i == 0) {
                cv::circle(strokeMask, cv::Point(ix, iy), r, cv::Scalar(255), -1);
            } else {
                auto [plx, ply] = doc->mapCanvasPosToLayerPos(m_points[i - 1], lyr);
                int pix = std::clamp(static_cast<int>(std::round(plx)), 0, w - 1);
                int piy = std::clamp(static_cast<int>(std::round(ply)), 0, h - 1);
                cv::line(strokeMask, cv::Point(pix, piy), cv::Point(ix, iy), cv::Scalar(255), r * 2);
                cv::circle(strokeMask, cv::Point(ix, iy), r, cv::Scalar(255), -1);
            }
        }

        // Run Guided Filter Hair Matting
        cv::Mat refinedMask = Core::MaskProcessor::refineEdgeMatting(lyr->image, curMask, strokeMask, m_radius);

        doc->updateMask(refinedMask, lyr->id, "Refine Edge Hair Matting");
        LOG_INFO("Refine Edge applied. Size: " + std::to_string(m_size) + ", Radius: " + std::to_string(m_radius));
    }

    m_points.clear();
    if (m_canvas) m_canvas->updateViewport();
}

void RefineEdgeTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter) return;

    painter->setRenderHint(QPainter::Antialiasing);

    // Draw active stroke translucent cyan highlight
    if (m_isDrawing && m_points.size() > 1) {
        QPen strokePen(QColor(0, 230, 255, 120), m_size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(strokePen);
        for (size_t i = 1; i < m_points.size(); ++i) {
            painter->drawLine(m_points[i - 1], m_points[i]);
        }
    }

    // Draw brush outline cursor
    painter->setPen(QPen(QColor(0, 230, 255), 1.5, Qt::DashLine));
    painter->setBrush(QBrush(QColor(0, 230, 255, 30)));
    painter->drawEllipse(m_currentPos, m_size / 2.0, m_size / 2.0);
}

} // namespace Tools
} // namespace ImageCut
