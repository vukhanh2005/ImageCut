#include "tools/PolyLassoTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "core/Layer.h"
#include "utils/Logger.h"
#include <opencv2/imgproc.hpp>
#include <QPolygonF>
#include <cmath>

namespace ImageCut {
namespace Tools {

PolyLassoTool::PolyLassoTool(UI::CanvasView* canvas, const QString& mode)
    : BaseTool(canvas), m_mode(mode)
{}

void PolyLassoTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        cancelPolygon();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_points.empty()) {
        m_points.push_back(imgPos);
        m_currentHoverPos = imgPos;
        m_isPlacing = true;
    } else {
        // Check if clicking close to the starting point (Close Polygon)
        double dx = imgPos.x() - m_points[0].x();
        double dy = imgPos.y() - m_points[0].y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 15.0 && m_points.size() >= 3) {
            finishPolygon();
        } else {
            m_points.push_back(imgPos);
        }
    }

    if (m_canvas && m_canvas->viewport()) {
        m_canvas->viewport()->update();
    }
}

void PolyLassoTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    m_currentHoverPos = imgPos;
    if (m_isPlacing && m_canvas && m_canvas->viewport()) {
        m_canvas->viewport()->update();
    }
}

void PolyLassoTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    (void)event;
}

void PolyLassoTool::keyPress(QKeyEvent* event) {
    int key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (m_points.size() >= 3) {
            finishPolygon();
        }
    } else if (key == Qt::Key_Escape) {
        cancelPolygon();
    } else if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
        if (!m_points.empty()) {
            m_points.pop_back();
            if (m_points.empty()) m_isPlacing = false;
            if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
        }
    }
}

void PolyLassoTool::cancelPolygon() {
    m_points.clear();
    m_isPlacing = false;
    if (m_canvas && m_canvas->viewport()) {
        m_canvas->viewport()->update();
    }
}

void PolyLassoTool::finishPolygon() {
    if (m_points.size() < 3 || !m_canvas || !m_canvas->getDocument()) {
        cancelPolygon();
        return;
    }

    auto doc = m_canvas->getDocument();
    auto lyr = doc->getActiveLayer();

    if (lyr && !lyr->locked && !lyr->image.empty()) {
        int w = lyr->width();
        int h = lyr->height();

        // Convert canvas scene points to layer local coordinates
        std::vector<cv::Point> ptsNp;
        for (const auto& pt : m_points) {
            auto [lx, ly] = doc->mapCanvasPosToLayerPos(pt, lyr);
            int ix = std::clamp(static_cast<int>(std::round(lx)), 0, w - 1);
            int iy = std::clamp(static_cast<int>(std::round(ly)), 0, h - 1);
            ptsNp.push_back(cv::Point(ix, iy));
        }

        cv::Mat polyMask(h, w, CV_8UC1, cv::Scalar(0));
        std::vector<std::vector<cv::Point>> polys = { ptsNp };
        cv::fillPoly(polyMask, polys, cv::Scalar(255));

        cv::Mat mask;
        if (m_mode == "Keep") {
            // Keep mode: Keep inside polygon (255), Erase outside polygon (0)
            mask = polyMask.clone();
        } else {
            // Remove mode: Erase inside polygon (0), Keep outside polygon untouched
            if (lyr->mask.empty()) {
                mask = cv::Mat(h, w, CV_8UC1, cv::Scalar(255));
            } else {
                mask = lyr->mask.clone();
            }
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (polyMask.at<uint8_t>(y, x) > 0) {
                        mask.at<uint8_t>(y, x) = 0;
                    }
                }
            }
        }

        doc->updateMask(mask, lyr->id, "PolyLasso Select");
        LOG_INFO("PolyLasso selection applied. Mode: " + m_mode.toStdString());
    }

    m_points.clear();
    m_isPlacing = false;
    if (m_canvas) m_canvas->updateViewport();
}

void PolyLassoTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter || m_points.empty()) return;

    painter->setRenderHint(QPainter::Antialiasing);

    QPolygonF poly;
    for (const auto& pt : m_points) poly.append(pt);

    // Draw main line segments
    QPen linePen(QColor(0, 230, 255), 2, Qt::SolidLine);
    painter->setPen(linePen);
    painter->drawPolyline(poly);

    // Draw rubberband line to current mouse position
    if (m_isPlacing) {
        QPen rubberPen(QColor(255, 170, 0), 1.5, Qt::DashLine);
        painter->setPen(rubberPen);
        painter->drawLine(m_points.back(), m_currentHoverPos);

        // Check if cursor is near starting point for magnetic snap indicator
        double dx = m_currentHoverPos.x() - m_points[0].x();
        double dy = m_currentHoverPos.y() - m_points[0].y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 15.0 && m_points.size() >= 3) {
            // Draw magnetic closing circle around start point
            painter->setPen(QPen(QColor(0, 255, 128), 2));
            painter->setBrush(QBrush(QColor(0, 255, 128, 120)));
            painter->drawEllipse(m_points[0], 10, 10);
        } else {
            // Draw regular start point indicator
            painter->setPen(QPen(QColor(255, 255, 255), 1.5));
            painter->setBrush(QBrush(QColor(0, 200, 255)));
            painter->drawEllipse(m_points[0], 6, 6);
        }

        // Draw vertex points
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(QColor(255, 170, 0)));
        for (size_t i = 1; i < m_points.size(); ++i) {
            painter->drawEllipse(m_points[i], 4, 4);
        }
    }
}

} // namespace Tools
} // namespace ImageCut
