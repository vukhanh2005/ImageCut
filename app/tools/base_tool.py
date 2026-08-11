from abc import ABC, abstractmethod
from PySide6.QtCore import QPointF
from PySide6.QtGui import QMouseEvent, QKeyEvent, QPainter

class BaseTool(ABC):
    """Abstract base class for canvas interactive tools."""

    def __init__(self, canvas):
        self.canvas = canvas
        self.cursor_shape = None

    @abstractmethod
    def mouse_press(self, img_pos: QPointF, event: QMouseEvent):
        pass

    @abstractmethod
    def mouse_move(self, img_pos: QPointF, event: QMouseEvent):
        pass

    @abstractmethod
    def mouse_release(self, img_pos: QPointF, event: QMouseEvent):
        pass

    def draw_overlay(self, painter: QPainter):
        """Draws custom tool overlays over canvas (e.g. brush circle, crop box)."""
        pass

    def key_press(self, event: QKeyEvent):
        pass
