#include "tools/MagicWandTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "core/Layer.h"
#include <opencv2/imgproc.hpp>
#include <QPen>
#include <QColor>
#include <cmath>

namespace ImageCut {
namespace Tools {

MagicWandTool::MagicWandTool(UI::CanvasView* canvas) : BaseTool(canvas) {}

void MagicWandTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !m_canvas || !m_canvas->getDocument()) return;

    auto doc = m_canvas->getDocument();
    auto lyr = doc->getActiveLayer();
    if (!lyr || lyr->image.empty()) return;

    cv::Mat img = lyr->image;
    int h = img.rows;
    int w = img.cols;

    auto [lx, ly] = doc->mapCanvasPosToLayerPos(imgPos, lyr);
    int seedX = static_cast<int>(std::round(lx));
    int seedY = static_cast<int>(std::round(ly));

    if (seedX < 0 || seedX >= w || seedY < 0 || seedY >= h) return;

    cv::Mat mask = !lyr->mask.empty() ? lyr->mask.clone() : cv::Mat(h, w, CV_8UC1, cv::Scalar(255));
    cv::Mat selectedRegion(h, w, CV_8UC1, cv::Scalar(0));

    if (m_contiguous) {
        cv::Mat ffMask(h + 2, w + 2, CV_8UC1, cv::Scalar(0));
        int flags = 4 | (255 << 8) | cv::FLOODFILL_MASK_ONLY | cv::FLOODFILL_FIXED_RANGE;
        cv::Scalar diff(m_tolerance, m_tolerance, m_tolerance);

        cv::Mat rgbImg;
        if (img.channels() == 4) {
            cv::cvtColor(img, rgbImg, cv::COLOR_RGBA2RGB);
        } else {
            rgbImg = img;
        }

        cv::floodFill(rgbImg, ffMask, cv::Point(seedX, seedY), cv::Scalar(0), nullptr, diff, diff, flags);
        selectedRegion = ffMask(cv::Rect(1, 1, w, h)).clone();
    } else {
        cv::Mat rgbImg;
        if (img.channels() == 4) {
            cv::cvtColor(img, rgbImg, cv::COLOR_RGBA2RGB);
        } else {
            rgbImg = img;
        }

        cv::Vec3b seedColor = rgbImg.at<cv::Vec3b>(seedY, seedX);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                cv::Vec3b c = rgbImg.at<cv::Vec3b>(y, x);
                double dist = std::sqrt(
                    (c[0] - seedColor[0]) * (c[0] - seedColor[0]) +
                    (c[1] - seedColor[1]) * (c[1] - seedColor[1]) +
                    (c[2] - seedColor[2]) * (c[2] - seedColor[2])
                );
                if (dist <= m_tolerance) {
                    selectedRegion.at<uint8_t>(y, x) = 255;
                }
            }
        }
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (selectedRegion.at<uint8_t>(y, x) > 0) {
                mask.at<uint8_t>(y, x) = 0;
            }
        }
    }

    doc->updateMask(mask, lyr->id, "Magic Wand Select");
    m_canvas->updateViewport();
}

void MagicWandTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos; (void)event;
}

void MagicWandTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos; (void)event;
}

void MagicWandTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter) return;
    QPointF pos = m_canvas->getHoverPos();
    if (pos.x() >= 0 && pos.y() >= 0) {
        painter->setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(255, 255, 0), 1.5, Qt::DashLine);
        painter->setPen(pen);
        painter->drawEllipse(pos, 8, 8);
        painter->drawLine(QPointF(pos.x() - 12, pos.y()), QPointF(pos.x() + 12, pos.y()));
        painter->drawLine(QPointF(pos.x(), pos.y() - 12), QPointF(pos.x(), pos.y() + 12));
    }
}

} // namespace Tools
} // namespace ImageCut
