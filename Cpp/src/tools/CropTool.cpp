#include "tools/CropTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include <QPen>
#include <QColor>
#include <QBrush>
#include <algorithm>

namespace ImageCut {
namespace Tools {

CropTool::CropTool(UI::CanvasView* canvas) : BaseTool(canvas) {}

void CropTool::setAspectRatio(const QString& ratioStr) {
    m_aspectRatio = ratioStr;
}

void CropTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_startPos = imgPos;
        m_cropRect = QRectF(imgPos, imgPos);
        if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void CropTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    if (m_isDragging) {
        double w = imgPos.x() - m_startPos.x();
        double h = imgPos.y() - m_startPos.y();

        if (m_aspectRatio == "1:1") {
            double sz = std::max(std::abs(w), std::abs(h));
            w = (w >= 0) ? sz : -sz;
            h = (h >= 0) ? sz : -sz;
        } else if (m_aspectRatio == "16:9") {
            h = w * (9.0 / 16.0);
        } else if (m_aspectRatio == "4:3") {
            h = w * (3.0 / 4.0);
        }

        m_cropRect = QRectF(m_startPos.x(), m_startPos.y(), w, h).normalized();
        if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void CropTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        if (m_canvas && m_canvas->viewport()) m_canvas->viewport()->update();
    }
}

void CropTool::applyCrop() {
    if (!m_cropRect.isEmpty() && m_canvas && m_canvas->getDocument()) {
        auto doc = m_canvas->getDocument();
        int w = static_cast<int>(m_cropRect.width());
        int h = static_cast<int>(m_cropRect.height());

        int oldW = doc->canvasWidth;
        int oldH = doc->canvasHeight;
        int newW = w;
        int newH = h;

        auto undoFn = [doc, oldW, oldH]() {
            doc->setCanvasSize(oldW, oldH);
        };
        auto redoFn = [doc, newW, newH]() {
            doc->setCanvasSize(newW, newH);
        };

        auto cmd = std::make_unique<Core::DocumentActionCommand>(doc.get(), undoFn, redoFn, "Crop Canvas");
        doc->undoStack.push(std::move(cmd));

        m_cropRect = QRectF();
        m_canvas->updateViewport();
    }
}

void CropTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter || m_cropRect.isEmpty() || !m_canvas->getDocument()) return;

    painter->setRenderHint(QPainter::Antialiasing);

    double docW = m_canvas->getDocument()->canvasWidth;
    double docH = m_canvas->getDocument()->canvasHeight;

    QBrush dimBrush(QColor(0, 0, 0, 140));
    painter->setBrush(dimBrush);
    painter->setPen(Qt::NoPen);

    painter->drawRect(QRectF(0, 0, docW, m_cropRect.top()));
    painter->drawRect(QRectF(0, m_cropRect.bottom(), docW, docH - m_cropRect.bottom()));
    painter->drawRect(QRectF(0, m_cropRect.top(), m_cropRect.left(), m_cropRect.height()));
    painter->drawRect(QRectF(m_cropRect.right(), m_cropRect.top(), docW - m_cropRect.right(), m_cropRect.height()));

    QPen pen(QColor(255, 255, 255), 2, Qt::SolidLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_cropRect);

    QPen gridPen(QColor(255, 255, 255, 120), 1, Qt::DashLine);
    painter->setPen(gridPen);
    double w3 = m_cropRect.width() / 3.0;
    double h3 = m_cropRect.height() / 3.0;
    for (int i = 1; i <= 2; ++i) {
        painter->drawLine(QPointF(m_cropRect.left() + w3 * i, m_cropRect.top()), QPointF(m_cropRect.left() + w3 * i, m_cropRect.bottom()));
        painter->drawLine(QPointF(m_cropRect.left(), m_cropRect.top() + h3 * i), QPointF(m_cropRect.right(), m_cropRect.top() + h3 * i));
    }
}

} // namespace Tools
} // namespace ImageCut
