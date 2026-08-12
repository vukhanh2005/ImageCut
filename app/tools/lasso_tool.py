from typing import List
import numpy as np
import cv2
from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QMouseEvent, QPainter, QPen, QColor, QPolygonF
from app.tools.base_tool import BaseTool

class LassoTool(BaseTool):
    """
    Polygon & Freehand Lasso tool for defining manual selection regions to erase or keep.
    Mode: "Remove" (erase inside lasso) or "Keep" (restore inside lasso).
    """

    def __init__(self, canvas, mode: str = "Remove"):
        super().__init__(canvas)
        self.mode = mode               # "Remove" or "Keep"
        self.points: List[QPointF] = []
        self.is_selecting = False

    def mouse_press(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_selecting = True
            self.points = [img_pos]
            self.canvas.update()

    def mouse_move(self, img_pos: QPointF, event: QMouseEvent):
        if self.is_selecting:
            self.points.append(img_pos)
            self.canvas.update()

    def mouse_release(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self.is_selecting:
            self.is_selecting = False
            doc = self.canvas.document
            lyr = doc.active_layer if doc else None
            if doc and lyr and lyr.image is not None and len(self.points) > 2:
                h, w = lyr.height(), lyr.width()
                mask = lyr.mask.copy() if lyr.mask is not None else np.full((h, w), 255, dtype=np.uint8)

                # Convert canvas points to local layer mask coordinates
                layer_pts = [doc.map_canvas_pos_to_layer_pos(p, lyr) for p in self.points]
                pts_np = np.array([[int(round(lx)), int(round(ly))] for lx, ly in layer_pts], dtype=np.int32)

                poly_mask = np.zeros((h, w), dtype=np.uint8)
                cv2.fillPoly(poly_mask, [pts_np], 255)

                if self.mode == "Remove":
                    mask[poly_mask > 0] = 0
                else:
                    mask[poly_mask > 0] = 255

                doc.update_mask(mask, layer_id=lyr.id, description="Lasso Select")

            self.points.clear()
            self.canvas.update()


    def draw_overlay(self, painter: QPainter):
        """Draws marching ant polyline during lasso selection."""
        if len(self.points) > 1:
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            pen = QPen(QColor(255, 128, 0), 2, Qt.PenStyle.DashLine)
            painter.setPen(pen)
            poly = QPolygonF(self.points)
            painter.drawPolyline(poly)
