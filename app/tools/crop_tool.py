from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QMouseEvent, QPainter, QPen, QColor, QBrush
from app.tools.base_tool import BaseTool

class CropTool(BaseTool):
    """
    Interactive Crop tool with visual crop bounding box and sizing handles.
    Supports aspect ratios: Free, 1:1, 4:3, 3:4, 16:9, 9:16.
    """

    def __init__(self, canvas):
        super().__init__(canvas)
        self.crop_rect: QRectF = None
        self.aspect_ratio: str = "Free" # "Free", "1:1", "4:3", "3:4", "16:9", "9:16"
        self.is_dragging = False
        self.start_pos = None

    def set_aspect_ratio(self, ratio_str: str):
        self.aspect_ratio = ratio_str

    def mouse_press(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_dragging = True
            self.start_pos = img_pos
            self.crop_rect = QRectF(img_pos, img_pos)
            self.canvas.update()

    def mouse_move(self, img_pos: QPointF, event: QMouseEvent):
        if self.is_dragging and self.start_pos is not None:
            w = img_pos.x() - self.start_pos.x()
            h = img_pos.y() - self.start_pos.y()

            # Enforce aspect ratio if specified
            if self.aspect_ratio == "1:1":
                size = max(abs(w), abs(h))
                w = size if w >= 0 else -size
                h = size if h >= 0 else -size
            elif self.aspect_ratio == "16:9":
                h = w * (9.0 / 16.0)
            elif self.aspect_ratio == "4:3":
                h = w * (3.0 / 4.0)

            self.crop_rect = QRectF(self.start_pos.x(), self.start_pos.y(), w, h).normalized()
            self.canvas.update()

    def mouse_release(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton and self.is_dragging:
            self.is_dragging = False
            self.canvas.update()

    def apply_crop(self):
        """Applies current crop box to document."""
        if self.crop_rect and self.canvas.document:
            doc = self.canvas.document
            r = self.crop_rect
            crop_tuple = (int(r.x()), int(r.y()), int(r.width()), int(r.height()))
            doc.crop_rect = crop_tuple
            doc.notify_changed()
            self.crop_rect = None
            self.canvas.update()

    def draw_overlay(self, painter: QPainter):
        """Draws crop rectangle handles and dimming mask around crop area."""
        if self.crop_rect and not self.crop_rect.isEmpty():
            painter.setRenderHint(QPainter.RenderHint.Antialiasing)

            # Dim outside region
            doc_rect = QRectF(0, 0, self.canvas.document.width(), self.canvas.document.height())
            dim_brush = QBrush(QColor(0, 0, 0, 140))
            painter.setBrush(dim_brush)
            painter.setPen(Qt.PenStyle.NoPen)

            # Draw 4 dimming rects around crop box
            painter.drawRect(QRectF(0, 0, doc_rect.width(), self.crop_rect.top()))
            painter.drawRect(QRectF(0, self.crop_rect.bottom(), doc_rect.width(), doc_rect.height() - self.crop_rect.bottom()))
            painter.drawRect(QRectF(0, self.crop_rect.top(), self.crop_rect.left(), self.crop_rect.height()))
            painter.drawRect(QRectF(self.crop_rect.right(), self.crop_rect.top(), doc_rect.width() - self.crop_rect.right(), self.crop_rect.height()))

            # Crop border
            pen = QPen(QColor(255, 255, 255), 2, Qt.PenStyle.SolidLine)
            painter.setPen(pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)
            painter.drawRect(self.crop_rect)

            # Rule of thirds grid lines
            grid_pen = QPen(QColor(255, 255, 255, 120), 1, Qt.PenStyle.DashLine)
            painter.setPen(grid_pen)
            w3 = self.crop_rect.width() / 3.0
            h3 = self.crop_rect.height() / 3.0
            for i in range(1, 3):
                painter.drawLine(int(self.crop_rect.left() + w3 * i), int(self.crop_rect.top()), int(self.crop_rect.left() + w3 * i), int(self.crop_rect.bottom()))
                painter.drawLine(int(self.crop_rect.left()), int(self.crop_rect.top() + h3 * i), int(self.crop_rect.right()), int(self.crop_rect.top() + h3 * i))
