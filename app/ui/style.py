DARK_THEME_QSS = """
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
"""

def apply_theme(app, theme_name: str = "Dark"):
    """Applies QSS stylesheet to application."""
    if theme_name == "Dark":
        app.setStyleSheet(DARK_THEME_QSS)
    else:
        app.setStyleSheet("")
