import numpy as np
import cv2
from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QMouseEvent, QPainter, QPen, QColor
from app.tools.base_tool import BaseTool

class MaskBrushTool(BaseTool):
    """
    Brush/Eraser tool for painting directly onto the alpha mask.
    Mode: "Restore" (value=255) or "Eraser" (value=0).
    """

    def __init__(self, canvas, mode: str = "Restore"):
        super().__init__(canvas)
        self.mode = mode               # "Restore" or "Eraser"
        self.size = 30                 # Brush diameter in pixels
        self.hardness = 0.8            # 0.0 to 1.0
        self.opacity = 1.0             # 0.0 to 1.0
        self.is_drawing = False
        self.last_pos = None
        self.current_mask = None

    def set_mode(self, mode: str):
        self.mode = mode

    def mouse_press(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() != Qt.MouseButton.LeftButton or not self.canvas.document:
            return

        self.is_drawing = True
        self.last_pos = (int(img_pos.x()), int(img_pos.y()))
        doc = self.canvas.document

        if doc.mask is None:
            doc.mask = np.full((doc.height(), doc.width()), 255, dtype=np.uint8)

        self.current_mask = doc.mask.copy()
        self._paint_circle(self.last_pos)
        self.canvas.update()

    def mouse_move(self, img_pos: QPointF, event: QMouseEvent):
        curr_pos = (int(img_pos.x()), int(img_pos.y()))
        if self.is_drawing and self.current_mask is not None:
            if self.last_pos is not None:
                self._paint_line(self.last_pos, curr_pos)
            self.last_pos = curr_pos
        self.canvas.update()

    def mouse_release(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self.is_drawing:
            self.is_drawing = False
            if self.canvas.document and self.current_mask is not None:
                description = "Restore Brush" if self.mode == "Restore" else "Erase Brush"
                self.canvas.document.update_mask(self.current_mask, description=description)
            self.current_mask = None
            self.last_pos = None
            self.canvas.update()

    def _paint_circle(self, center: tuple):
        if self.current_mask is None:
            return

        cx, cy = center
        r = max(1, self.size // 2)
        val = 255 if self.mode == "Restore" else 0
        h, w = self.current_mask.shape[:2]

        # Draw circle on mask
        cv2.circle(self.current_mask, (cx, cy), r, (val,), -1)

    def _paint_line(self, p1: tuple, p2: tuple):
        if self.current_mask is None:
            return

        r = max(1, self.size // 2)
        val = 255 if self.mode == "Restore" else 0
        cv2.line(self.current_mask, p1, p2, (val,), thickness=r * 2)

    def draw_overlay(self, painter: QPainter):
        """Draws brush radius preview ring around current mouse position."""
        if self.canvas.hover_img_pos is not None:
            pos = self.canvas.hover_img_pos
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)

            pen_color = QColor(0, 255, 128) if self.mode == "Restore" else QColor(255, 64, 64)
            pen = QPen(pen_color, 1.5, Qt.PenStyle.SolidLine)
            painter.setPen(pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)

            r = self.size / 2.0
            painter.drawEllipse(pos, r, r)
