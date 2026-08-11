import sys
import os

# Ensure project root is in sys.path
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from PySide6.QtWidgets import QApplication
from PySide6.QtCore import Qt
from app.ui.main_window import MainWindow
from app.ui.style import apply_theme
from app.utils.logger import logger
from app.utils.settings import settings

def main():
    logger.info("Starting Personal Background Remover & Image Editor...")

    # Enable High DPI Scaling
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )

    app = QApplication(sys.argv)
    app.setApplicationName("BackgroundRemover")
    app.setOrganizationName("ImageCut")

    # Apply Dark Theme stylesheet
    theme_name = settings.get("theme", "Dark")
    apply_theme(app, theme_name)

    window = MainWindow()
    window.show()

    sys.exit(app.exec())

if __name__ == "__main__":
    main()
