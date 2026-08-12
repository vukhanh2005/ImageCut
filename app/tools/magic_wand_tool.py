import numpy as np
import cv2
from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QMouseEvent, QPainter, QPen, QColor
from app.tools.base_tool import BaseTool

class MagicWandTool(BaseTool):
    """
    Smart Color Selection / Magic Wand tool.
    Clicking a location flood-fills pixels with similar color within tolerance
    and updates the alpha mask (erasing background).
    """

    def __init__(self, canvas):
        super().__init__(canvas)
        self.tolerance = 30        # 1 to 100
        self.contiguous = True     # Flood fill vs global color selection
        self.feather = 2

    def mouse_press(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() != Qt.MouseButton.LeftButton or not self.canvas.document:
            return

        doc = self.canvas.document
        lyr = doc.active_layer
        if not lyr or lyr.image is None:
            return

        img = lyr.image
        h, w = img.shape[:2]

        layer_pos = doc.map_canvas_pos_to_layer_pos(img_pos, lyr)
        seed_x, seed_y = int(round(layer_pos[0])), int(round(layer_pos[1]))

        if seed_x < 0 or seed_x >= w or seed_y < 0 or seed_y >= h:
            return

        # Prepare new mask for active layer
        mask = lyr.mask.copy() if lyr.mask is not None else np.full((h, w), 255, dtype=np.uint8)

        if self.contiguous:
            # OpenCV Flood Fill
            ff_mask = np.zeros((h + 2, w + 2), dtype=np.uint8)
            flags = 4 | (255 << 8) | cv2.FLOODFILL_MASK_ONLY | cv2.FLOODFILL_FIXED_RANGE
            lo_diff = (self.tolerance, self.tolerance, self.tolerance)
            up_diff = (self.tolerance, self.tolerance, self.tolerance)

            cv2.floodFill(img, ff_mask, (seed_x, seed_y), 0, lo_diff, up_diff, flags)
            selected_region = ff_mask[1:h+1, 1:w+1] > 0
        else:
            # Global color distance in LAB space
            seed_color = img[seed_y, seed_x].astype(np.float32)
            dist = np.linalg.norm(img.astype(np.float32) - seed_color, axis=2)
            selected_region = dist <= self.tolerance

        # Erase selected region from layer mask
        mask[selected_region] = 0

        # Push command
        doc.update_mask(mask, layer_id=lyr.id, description="Magic Wand Select")
        self.canvas.update()


    def mouse_move(self, img_pos: QPointF, event: QMouseEvent):
        pass

    def mouse_release(self, img_pos: QPointF, event: QMouseEvent):
        pass

    def draw_overlay(self, painter: QPainter):
        """Draws small target crosshair cursor for magic wand."""
        if self.canvas.hover_img_pos is not None:
            pos = self.canvas.hover_img_pos
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)
            pen = QPen(QColor(255, 255, 0), 1.5, Qt.PenStyle.DashLine)
            painter.setPen(pen)
            painter.drawEllipse(pos, 8, 8)
            painter.drawLine(int(pos.x() - 12), int(pos.y()), int(pos.x() + 12), int(pos.y()))
            painter.drawLine(int(pos.x()), int(pos.y() - 12), int(pos.x()), int(pos.y() + 12))
