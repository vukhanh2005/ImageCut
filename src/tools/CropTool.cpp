#include "tools/CropTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "utils/Logger.h"
#include <QPen>
#include <QColor>
#include <QBrush>
#include <cmath>
#include <algorithm>

namespace ImageCut {
namespace Tools {

CropTool::CropTool(UI::CanvasView* canvas) : BaseTool(canvas) {}

void CropTool::setAspectRatio(const QString& ratioStr) {
    m_aspectRatio = ratioStr;
}

std::vector<QRectF> CropTool::getHandleRects() const {
    if (m_cropRect.isEmpty()) return {};

    double s = 10.0;
    double l = m_cropRect.left();
    double t = m_cropRect.top();
    double r = m_cropRect.right();
    double b = m_cropRect.bottom();
    double cx = m_cropRect.center().x();
    double cy = m_cropRect.center().y();

    return {
        QRectF(l - s / 2, t - s / 2, s, s),   // TopLeft
        QRectF(cx - s / 2, t - s / 2, s, s),  // TopCenter
        QRectF(r - s / 2, t - s / 2, s, s),   // TopRight
        QRectF(l - s / 2, cy - s / 2, s, s),  // MiddleLeft
        QRectF(r - s / 2, cy - s / 2, s, s),  // MiddleRight
        QRectF(l - s / 2, b - s / 2, s, s),   // BottomLeft
        QRectF(cx - s / 2, b - s / 2, s, s),  // BottomCenter
        QRectF(r - s / 2, b - s / 2, s, s)    // BottomRight
    };
}

CropTool::HandleType CropTool::hitTest(const QPointF& pos) const {
    if (m_cropRect.isEmpty()) return None;

    auto handles = getHandleRects();
    if (handles.size() == 8) {
        if (handles[0].contains(pos)) return TopLeft;
        if (handles[1].contains(pos)) return TopCenter;
        if (handles[2].contains(pos)) return TopRight;
        if (handles[3].contains(pos)) return MiddleLeft;
        if (handles[4].contains(pos)) return MiddleRight;
        if (handles[5].contains(pos)) return BottomLeft;
        if (handles[6].contains(pos)) return BottomCenter;
        if (handles[7].contains(pos)) return BottomRight;
    }

    if (m_cropRect.contains(pos)) return Body;
    return None;
}

void CropTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    m_activeHandle = hitTest(imgPos);
    m_startPos = imgPos;
    m_initialCropRect = m_cropRect;

    if (m_activeHandle == None) {
        m_cropRect = QRectF(imgPos, imgPos);
        m_activeHandle = BottomRight;
    }
    m_isDragging = true;
    if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
}

void CropTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    if (!m_isDragging) return;

    QPointF delta = imgPos - m_startPos;
    QRectF r = m_initialCropRect;

    if (r.isEmpty() && m_activeHandle == BottomRight) {
        r = QRectF(m_startPos, imgPos).normalized();
    } else {
        switch (m_activeHandle) {
        case Body:
            r.translate(delta);
            break;
        case TopLeft:
            r.setTopLeft(r.topLeft() + delta);
            break;
        case TopCenter:
            r.setTop(r.top() + delta.y());
            break;
        case TopRight:
            r.setTopRight(r.topRight() + delta);
            break;
        case MiddleLeft:
            r.setLeft(r.left() + delta.x());
            break;
        case MiddleRight:
            r.setRight(r.right() + delta.x());
            break;
        case BottomLeft:
            r.setBottomLeft(r.bottomLeft() + delta);
            break;
        case BottomCenter:
            r.setBottom(r.bottom() + delta.y());
            break;
        case BottomRight:
            r.setBottomRight(r.bottomRight() + delta);
            break;
        default:
            break;
        }
    }

    r = r.normalized();

    // Aspect ratio constraints
    if (m_aspectRatio == "1:1 Square") {
        double side = std::max(r.width(), r.height());
        r.setWidth(side);
        r.setHeight(side);
    } else if (m_aspectRatio == "16:9 Landscape") {
        r.setHeight(r.width() * (9.0 / 16.0));
    } else if (m_aspectRatio == "9:16 Portrait / Story") {
        r.setHeight(r.width() * (16.0 / 9.0));
    } else if (m_aspectRatio == "4:3 Standard") {
        r.setHeight(r.width() * (3.0 / 4.0));
    }

    m_cropRect = r;
    if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
}

void CropTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        m_activeHandle = None;
        if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void CropTool::applyCrop() {
    if (m_cropRect.isEmpty() || !m_canvas || !m_canvas->getDocument()) return;

    auto doc = m_canvas->getDocument();
    QRectF normRect = m_cropRect.normalized();

    double cropX = normRect.x();
    double cropY = normRect.y();
    int newW = std::max(1, static_cast<int>(std::round(normRect.width())));
    int newH = std::max(1, static_cast<int>(std::round(normRect.height())));

    int oldW = doc->canvasWidth;
    int oldH = doc->canvasHeight;

    struct LayerOffsetState {
        QString id;
        double oldX, oldY;
        double newX, newY;
    };
    std::vector<LayerOffsetState> layerStates;

    for (const auto& lyr : doc->layers) {
        if (!lyr) continue;
        layerStates.push_back({
            lyr->id,
            lyr->offsetX,
            lyr->offsetY,
            lyr->offsetX - cropX,
            lyr->offsetY - cropY
        });
    }

    auto undoFn = [doc, oldW, oldH, layerStates]() {
        doc->setCanvasSize(oldW, oldH);
        for (const auto& st : layerStates) {
            auto lyr = doc->getLayerById(st.id);
            if (lyr) {
                lyr->offsetX = st.oldX;
                lyr->offsetY = st.oldY;
                lyr->invalidateCache();
            }
        }
        doc->notifyChanged();
    };

    auto redoFn = [doc, newW, newH, layerStates]() {
        doc->setCanvasSize(newW, newH);
        for (const auto& st : layerStates) {
            auto lyr = doc->getLayerById(st.id);
            if (lyr) {
                lyr->offsetX = st.newX;
                lyr->offsetY = st.newY;
                lyr->invalidateCache();
            }
        }
        doc->notifyChanged();
    };

    redoFn();

    auto cmd = std::make_unique<Core::DocumentActionCommand>(doc.get(), undoFn, redoFn, "Crop Canvas");
    doc->undoStack.push(std::move(cmd));

    m_cropRect = QRectF();
    LOG_INFO("Applied Crop Canvas: " + std::to_string(newW) + "x" + std::to_string(newH) + " at (" + std::to_string(cropX) + ", " + std::to_string(cropY) + ")");
    m_canvas->updateViewport();
}

void CropTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter || m_cropRect.isEmpty() || !m_canvas->getDocument()) return;

    painter->setRenderHint(QPainter::Antialiasing);

    double docW = m_canvas->getDocument()->canvasWidth;
    double docH = m_canvas->getDocument()->canvasHeight;
    QRectF normRect = m_cropRect.normalized();

    QBrush dimBrush(QColor(0, 0, 0, 160));
    painter->setBrush(dimBrush);
    painter->setPen(Qt::NoPen);

    // Dim regions outside crop box
    painter->drawRect(QRectF(0, 0, docW, normRect.top()));
    painter->drawRect(QRectF(0, normRect.bottom(), docW, docH - normRect.bottom()));
    painter->drawRect(QRectF(0, normRect.top(), normRect.left(), normRect.height()));
    painter->drawRect(QRectF(normRect.right(), normRect.top(), docW - normRect.right(), normRect.height()));

    // Crop border
    QPen pen(QColor(0, 230, 255), 2, Qt::SolidLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(normRect);

    // Rule of Thirds grid lines
    QPen gridPen(QColor(255, 255, 255, 140), 1, Qt::DashLine);
    painter->setPen(gridPen);
    double w3 = normRect.width() / 3.0;
    double h3 = normRect.height() / 3.0;
    for (int i = 1; i <= 2; ++i) {
        painter->drawLine(QPointF(normRect.left() + w3 * i, normRect.top()), QPointF(normRect.left() + w3 * i, normRect.bottom()));
        painter->drawLine(QPointF(normRect.left(), normRect.top() + h3 * i), QPointF(normRect.right(), normRect.top() + h3 * i));
    }

    // Draw 8 handle control points
    auto handles = getHandleRects();
    painter->setPen(QPen(QColor(0, 230, 255), 1.5));
    painter->setBrush(QBrush(QColor(255, 255, 255)));
    for (const auto& h : handles) {
        painter->drawRect(h);
    }
}

} // namespace Tools
} // namespace ImageCut
