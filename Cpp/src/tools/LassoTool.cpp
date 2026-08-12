#include "tools/LassoTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "core/Layer.h"
#include <opencv2/imgproc.hpp>
#include <QPen>
#include <QColor>
#include <QPolygonF>
#include <cmath>

namespace ImageCut {
namespace Tools {

LassoTool::LassoTool(UI::CanvasView* canvas, const QString& mode)
    : BaseTool(canvas), m_mode(mode) {}

void LassoTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isSelecting = true;
        m_points = { imgPos };
        if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void LassoTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    if (m_isSelecting) {
        m_points.push_back(imgPos);
        if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void LassoTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_isSelecting = false;
        if (m_canvas && m_canvas->getDocument()) {
            auto doc = m_canvas->getDocument();
            auto lyr = doc->getActiveLayer();
            if (lyr && !lyr->image.empty() && m_points.size() > 2) {
                int h = lyr->height();
                int w = lyr->width();

                std::vector<cv::Point> ptsNp;
                for (const auto& p : m_points) {
                    auto [lx, ly] = doc->mapCanvasPosToLayerPos(p, lyr);
                    ptsNp.push_back(cv::Point(static_cast<int>(std::round(lx)), static_cast<int>(std::round(ly))));
                }

                cv::Mat polyMask(h, w, CV_8UC1, cv::Scalar(0));
                std::vector<std::vector<cv::Point>> polys = { ptsNp };
                cv::fillPoly(polyMask, polys, cv::Scalar(255));

                cv::Mat mask;
                if (m_mode == "Keep") {
                    mask = polyMask.clone();
                } else {
                    mask = !lyr->mask.empty() ? lyr->mask.clone() : cv::Mat(h, w, CV_8UC1, cv::Scalar(255));
                    for (int y = 0; y < h; ++y) {
                        for (int x = 0; x < w; ++x) {
                            if (polyMask.at<uint8_t>(y, x) > 0) {
                                mask.at<uint8_t>(y, x) = 0;
                            }
                        }
                    }
                }

                doc->updateMask(mask, lyr->id, "Lasso Select");
            }
        }
        m_points.clear();
        m_canvas->updateViewport();
    }
}

void LassoTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter || m_points.size() <= 1) return;

    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(255, 128, 0), 2, Qt::DashLine);
    painter->setPen(pen);
    QPolygonF poly;
    for (const auto& pt : m_points) poly.append(pt);
    painter->drawPolyline(poly);
}

} // namespace Tools
} // namespace ImageCut
