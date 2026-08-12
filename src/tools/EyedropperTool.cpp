#include "tools/EyedropperTool.h"
#include "ui/CanvasView.h"
#include "core/ImageDocument.h"
#include "processing/Compositor.h"
#include "utils/Logger.h"
#include <cmath>
#include <QFont>
#include <QFontMetrics>

namespace ImageCut {
namespace Tools {

EyedropperTool::EyedropperTool(UI::CanvasView* canvas)
    : BaseTool(canvas)
{}

void EyedropperTool::mousePress(const QPointF& imgPos, QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !m_canvas || !m_canvas->getDocument()) return;

    auto doc = m_canvas->getDocument();
    int canvasW = doc->canvasWidth;
    int canvasH = doc->canvasHeight;

    cv::Mat comp = Processing::Compositor::compositeDocument(*doc, false, false);
    if (!comp.empty()) {
        int x = std::clamp(static_cast<int>(std::round(imgPos.x())), 0, comp.cols - 1);
        int y = std::clamp(static_cast<int>(std::round(imgPos.y())), 0, comp.rows - 1);

        cv::Vec4b px = comp.at<cv::Vec4b>(y, x);
        m_pickedColor = QColor(px[0], px[1], px[2], px[3]);

        LOG_INFO("Eyedropper color picked: " + m_pickedColor.name().toStdString());
        emit colorPickedSignal(m_pickedColor);
    }
}

void EyedropperTool::mouseMove(const QPointF& imgPos, QMouseEvent* event) {
    (void)event;
    m_currentPos = imgPos;
    m_hasHover = true;

    if (m_canvas && m_canvas->getDocument()) {
        auto doc = m_canvas->getDocument();
        cv::Mat comp = Processing::Compositor::compositeDocument(*doc, true, true);
        if (!comp.empty()) {
            int x = std::clamp(static_cast<int>(std::round(imgPos.x())), 0, comp.cols - 1);
            int y = std::clamp(static_cast<int>(std::round(imgPos.y())), 0, comp.rows - 1);
            cv::Vec4b px = comp.at<cv::Vec4b>(y, x);
            m_pickedColor = QColor(px[0], px[1], px[2], px[3]);
        }
    }

    if (m_canvas && m_canvas->viewport()) {
        m_canvas->viewport()->update();
    }
}

void EyedropperTool::mouseRelease(const QPointF& imgPos, QMouseEvent* event) {
    (void)imgPos;
    (void)event;
}

void EyedropperTool::drawOverlay(QPainter* painter) {
    if (!m_canvas || !painter || !m_hasHover) return;

    painter->setRenderHint(QPainter::Antialiasing);

    // Magnifier Ring radius
    double r = 32.0;
    QPointF center = m_currentPos;

    // Draw outer white border & dark shadow
    painter->setPen(QPen(QColor(30, 30, 30), 4));
    painter->setBrush(QBrush(m_pickedColor));
    painter->drawEllipse(center, r, r);

    painter->setPen(QPen(QColor(255, 255, 255), 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(center, r, r);

    // Draw Hex color text label below magnifier
    QString hexStr = m_pickedColor.name().toUpper();
    QFont font("Consolas", 10, QFont::Bold);
    painter->setFont(font);

    QFontMetrics fm(font);
    int txtW = fm.horizontalAdvance(hexStr) + 12;
    QRectF txtRect(center.x() - txtW / 2.0, center.y() + r + 8, txtW, 20);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor(20, 20, 30, 220)));
    painter->drawRoundedRect(txtRect, 4, 4);

    painter->setPen(QPen(Qt::white));
    painter->drawText(txtRect, Qt::AlignCenter, hexStr);
}

} // namespace Tools
} // namespace ImageCut
