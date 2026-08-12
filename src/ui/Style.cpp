#include "ui/Style.h"
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <vector>

namespace ImageCut {
namespace UI {

static const char* DARK_THEME_QSS = R"(
QMainWindow {
    background-color: #12131A;
    color: #E2E8F0;
}

QWidget {
    background-color: #12131A;
    color: #E2E8F0;
    font-family: 'Segoe UI', Arial, sans-serif;
    font-size: 13px;
}

QMenuBar {
    background-color: #181924;
    color: #CBD5E1;
    border-bottom: 1px solid #2D3748;
}

QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
    border-radius: 4px;
}

QMenuBar::item:selected {
    background-color: #2D3748;
    color: #FFFFFF;
}

QMenu {
    background-color: #1E202E;
    border: 1px solid #334155;
    padding: 4px;
    border-radius: 6px;
}

QMenu::item {
    padding: 6px 24px;
    border-radius: 4px;
}

QMenu::item:selected {
    background-color: #6C5CE7;
    color: #FFFFFF;
}

QToolBar {
    background-color: #181924;
    border-bottom: 1px solid #2D3748;
    spacing: 8px;
    padding: 6px;
}

QPushButton {
    background-color: #262838;
    color: #F8FAFC;
    border: 1px solid #3B4252;
    border-radius: 6px;
    padding: 6px 16px;
    font-weight: 500;
}

QPushButton:hover {
    background-color: #3B4252;
    border-color: #6C5CE7;
}

QPushButton:pressed {
    background-color: #6C5CE7;
    color: #FFFFFF;
}

QPushButton:disabled {
    background-color: #1E202E;
    color: #64748B;
    border-color: #262838;
}

QPushButton#btn_primary {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #6C5CE7, stop:1 #a29bfe);
    color: #FFFFFF;
    font-weight: bold;
    border: none;
}

QPushButton#btn_primary:hover {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5B4BC4, stop:1 #8C82FC);
}

QPushButton#btn_tool {
    background-color: #1E202E;
    border: 1px solid transparent;
    padding: 8px;
    border-radius: 6px;
}

QPushButton#btn_tool:hover {
    background-color: #2D3748;
    border-color: #6C5CE7;
}

QPushButton#btn_tool:checked {
    background-color: #6C5CE7;
    color: #FFFFFF;
    border-color: #A29BFE;
}

QTabWidget::pane {
    border: 1px solid #2D3748;
    background-color: #181924;
    border-radius: 6px;
}

QTabBar::tab {
    background-color: #1E202E;
    color: #94A3B8;
    padding: 8px 16px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
}

QTabBar::tab:selected {
    background-color: #181924;
    color: #A29BFE;
    font-weight: bold;
    border-bottom: 2px solid #6C5CE7;
}

QTabBar::tab:hover:!selected {
    background-color: #2D3748;
    color: #E2E8F0;
}

QSlider::groove:horizontal {
    height: 6px;
    background: #2D3748;
    border-radius: 3px;
}

QSlider::sub-page:horizontal {
    background: #6C5CE7;
    border-radius: 3px;
}

QSlider::handle:horizontal {
    background: #FFFFFF;
    border: 2px solid #6C5CE7;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}

QSlider::handle:horizontal:hover {
    background: #A29BFE;
}

QGroupBox {
    border: 1px solid #2D3748;
    border-radius: 6px;
    margin-top: 12px;
    padding-top: 12px;
    font-weight: bold;
    color: #A29BFE;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 6px;
}

QComboBox {
    background-color: #262838;
    border: 1px solid #3B4252;
    border-radius: 6px;
    padding: 4px 8px;
    color: #F8FAFC;
}

QComboBox:hover {
    border-color: #6C5CE7;
}

QComboBox QAbstractItemView {
    background-color: #1E202E;
    selection-background-color: #6C5CE7;
    border: 1px solid #334155;
}

QSpinBox, QDoubleSpinBox {
    background-color: #262838;
    border: 1px solid #3B4252;
    border-radius: 6px;
    padding: 4px;
    color: #F8FAFC;
}

QStatusBar {
    background-color: #181924;
    color: #94A3B8;
    border-top: 1px solid #2D3748;
}

QGraphicsView {
    border: none;
    background-color: #0F1015;
}
)";

void Style::applyTheme(QApplication* app, const QString& themeName) {
    if (!app) return;
    if (themeName == "Dark") {
        app->setStyleSheet(DARK_THEME_QSS);
    } else {
        app->setStyleSheet("");
    }
}

QIcon UIIcons::getIcon(const QString& name, const QColor& baseColor, int size) {
    auto renderPixmap = [name, size](const QColor& color) -> QPixmap {
        QPixmap pix(size * 2, size * 2);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.scale(size * 2 / 24.0, size * 2 / 24.0);

        QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        if (name == "select") {
            QPainterPath path;
            path.moveTo(4, 4);
            path.lineTo(4, 18);
            path.lineTo(8.5, 14);
            path.lineTo(12, 20);
            path.lineTo(14.5, 19);
            path.lineTo(11, 13);
            path.lineTo(16, 13);
            path.closeSubpath();
            p.setBrush(color);
            p.drawPath(path);
        } else if (name == "brush") {
            QPainterPath body;
            body.moveTo(18, 3);
            body.lineTo(21, 6);
            body.lineTo(10, 17);
            body.lineTo(6, 17);
            body.lineTo(6, 13);
            body.closeSubpath();
            p.drawPath(body);

            QPainterPath bristle;
            bristle.moveTo(6, 17);
            bristle.quadTo(3, 19, 4, 21);
            bristle.quadTo(7, 21, 10, 17);
            p.setBrush(color);
            p.drawPath(bristle);
        } else if (name == "eraser") {
            QPainterPath path;
            path.moveTo(6, 14);
            path.lineTo(14, 6);
            path.lineTo(20, 12);
            path.lineTo(12, 20);
            path.closeSubpath();
            p.drawPath(path);
            p.drawLine(4, 20, 20, 20);
        } else if (name == "refine_edge") {
            p.drawEllipse(QPointF(10, 10), 6, 6);
            p.drawLine(15, 15, 21, 21);
            p.drawLine(10, 2, 10, 4);
            p.drawLine(2, 10, 4, 10);
            p.drawLine(4, 4, 6, 6);
        } else if (name == "magic_wand") {
            p.drawLine(4, 20, 14, 10);
            QPainterPath star;
            star.moveTo(17, 3);
            star.quadTo(17, 7, 21, 7);
            star.quadTo(17, 7, 17, 11);
            star.quadTo(17, 7, 13, 7);
            star.quadTo(17, 7, 17, 3);
            p.setBrush(color);
            p.drawPath(star);
        } else if (name == "eyedropper") {
            QPainterPath path;
            path.moveTo(19, 5);
            path.lineTo(17, 3);
            path.lineTo(14, 6);
            path.lineTo(12, 5);
            path.lineTo(7, 10);
            path.lineTo(8, 12);
            path.lineTo(4, 16);
            path.lineTo(4, 20);
            path.lineTo(8, 20);
            path.lineTo(12, 16);
            path.lineTo(14, 17);
            path.lineTo(19, 12);
            path.lineTo(18, 10);
            path.closeSubpath();
            p.drawPath(path);
        } else if (name == "lasso") {
            QPainterPath path;
            path.moveTo(12, 4);
            path.cubicTo(20, 4, 21, 14, 15, 18);
            path.cubicTo(10, 21, 4, 16, 6, 10);
            path.cubicTo(8, 5, 12, 11, 14, 15);
            p.drawPath(path);
        } else if (name == "poly_lasso") {
            p.drawPolyline(std::vector<QPoint>{ {5, 18}, {6, 6}, {18, 4}, {20, 16}, {12, 20}, {5, 18} }.data(), 6);
            p.setBrush(color);
            p.drawEllipse(QPointF(5, 18), 2.5, 2.5);
            p.drawEllipse(QPointF(6, 6), 2.5, 2.5);
            p.drawEllipse(QPointF(18, 4), 2.5, 2.5);
            p.drawEllipse(QPointF(20, 16), 2.5, 2.5);
        } else if (name == "crop") {
            p.drawLine(6, 2, 6, 18);
            p.drawLine(6, 18, 22, 18);
            p.drawLine(18, 6, 18, 22);
            p.drawLine(2, 6, 18, 6);
        } else if (name == "ai_auto") {
            QPainterPath star1;
            star1.moveTo(10, 2);
            star1.quadTo(10, 8, 16, 8);
            star1.quadTo(10, 8, 10, 14);
            star1.quadTo(10, 8, 4, 8);
            star1.quadTo(10, 8, 10, 2);
            p.setBrush(color);
            p.drawPath(star1);

            QPainterPath star2;
            star2.moveTo(18, 13);
            star2.quadTo(18, 17, 22, 17);
            star2.quadTo(18, 17, 18, 21);
            star2.quadTo(18, 17, 14, 17);
            star2.quadTo(18, 17, 18, 13);
            p.drawPath(star2);
        } else if (name == "add_image") {
            p.drawRoundedRect(3, 4, 15, 13, 2, 2);
            p.drawEllipse(QPointF(7, 8), 2, 2);
            QPainterPath mtn;
            mtn.moveTo(5, 15);
            mtn.lineTo(9, 11);
            mtn.lineTo(12, 14);
            mtn.lineTo(14, 12);
            mtn.lineTo(16, 15);
            p.drawPath(mtn);
            p.drawLine(20, 14, 20, 20);
            p.drawLine(17, 17, 23, 17);
        } else if (name == "add_text") {
            p.drawLine(4, 5, 20, 5);
            p.drawLine(12, 5, 12, 20);
            p.drawLine(9, 20, 15, 20);
        } else if (name == "add_shape") {
            p.drawRect(3, 3, 11, 11);
            p.drawEllipse(QPointF(15, 15), 6, 6);
        } else if (name == "export") {
            p.drawPolyline(std::vector<QPoint>{ {4, 14}, {4, 20}, {20, 20}, {20, 14} }.data(), 4);
            p.drawLine(12, 15, 12, 3);
            p.drawLine(7, 8, 12, 3);
            p.drawLine(17, 8, 12, 3);
        } else if (name == "undo") {
            p.drawArc(4, 6, 16, 14, 30 * 16, 150 * 16);
            p.drawLine(4, 10, 4, 4);
            p.drawLine(4, 4, 10, 4);
        } else if (name == "redo") {
            p.drawArc(4, 6, 16, 14, 0 * 16, 150 * 16);
            p.drawLine(20, 10, 20, 4);
            p.drawLine(20, 4, 14, 4);
        }

        p.end();
        return pix;
    };

    QIcon icon;
    icon.addPixmap(renderPixmap(baseColor), QIcon::Normal, QIcon::Off);
    icon.addPixmap(renderPixmap(QColor(255, 255, 255)), QIcon::Active, QIcon::On);
    icon.addPixmap(renderPixmap(QColor(108, 92, 231)), QIcon::Selected, QIcon::On);
    return icon;
}

} // namespace UI
} // namespace ImageCut
