import pytest
from PySide6.QtWidgets import QApplication

@pytest.fixture(scope="session", autouse=True)
def qapp():
    """Session-wide PySide6 QApplication fixture."""
    app = QApplication.instance()
    if app is None:
        app = QApplication([])
    yield app
