from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QPainter, QPixmap, QWheelEvent, QMouseEvent, QDragEnterEvent, QDropEvent
from PySide6.QtWidgets import QGraphicsView, QGraphicsScene, QGraphicsPixmapItem
from app.core.image_document import ImageDocument
from app.processing.compositing import composite_document
from app.utils.image_utils import numpy_to_qpixmap
from app.utils.logger import logger

class CanvasView(QGraphicsView):
    """
    Interactive canvas graphics view supporting hardware accelerated display,
    zoom (10% to 1600%), pan, drag-and-drop, and live tool overlays.
    """
    mouse_moved_signal = Signal(QPointF, tuple) # (img_pos, pixel_rgb)
    image_dropped_signal = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.scene = QGraphicsScene(self)
        self.setScene(self.scene)

        self.document: ImageDocument = None
        self.pixmap_item: QGraphicsPixmapItem = None
        self.active_tool = None
        self.hover_img_pos: QPointF = None

        self.zoom_factor = 1.0
        self.is_space_panning = False
        self.pan_start_pos = None

        self.setRenderHints(QPainter.RenderHint.Antialiasing | QPainter.RenderHint.SmoothPixmapTransform)
        self.setViewportUpdateMode(QGraphicsView.ViewportUpdateMode.FullViewportUpdate)
        self.setMouseTracking(True)
        self.setAcceptDrops(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)

    def set_document(self, doc: ImageDocument):
        self.document = doc
        if self.document:
            self.document.add_change_listener(self.update_canvas)
        self.update_canvas()
        self.fit_in_view()

    def set_active_tool(self, tool):
        self.active_tool = tool
        self.viewport().update()

    def update_canvas(self):
        """Renders composite image and updates canvas scene."""
        if not self.document or self.document.original_image is None:
            self.scene.clear()
            self.pixmap_item = None
            return

        comp_rgba = composite_document(self.document, preview_mode=True)
        pixmap = numpy_to_qpixmap(comp_rgba)

        if self.pixmap_item is None:
            self.scene.clear()
            self.pixmap_item = self.scene.addPixmap(pixmap)
        else:
            self.pixmap_item.setPixmap(pixmap)

        self.scene.setSceneRect(0, 0, pixmap.width(), pixmap.height())
        self.viewport().update()

    def fit_in_view(self):
        """Fits current document to view viewport."""
        if self.scene and not self.scene.sceneRect().isEmpty():
            self.fitInView(self.scene.sceneRect(), Qt.AspectRatioMode.KeepAspectRatio)
            self.zoom_factor = self.transform().m11()

    def set_zoom_level(self, factor: float):
        """Sets explicit zoom scale factor."""
        self.resetTransform()
        self.scale(factor, factor)
        self.zoom_factor = factor

    def wheelEvent(self, event: QWheelEvent):
        """Zoom in/out with Ctrl + Wheel or Wheel alone."""
        zoom_in_factor = 1.15
        zoom_out_factor = 1 / zoom_in_factor

        if event.angleDelta().y() > 0:
            factor = zoom_in_factor
        else:
            factor = zoom_out_factor

        new_zoom = self.zoom_factor * factor
        if 0.05 <= new_zoom <= 20.0:
            self.scale(factor, factor)
            self.zoom_factor = new_zoom

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Space and not self.is_space_panning:
            self.is_space_panning = True
            self.setCursor(Qt.CursorShape.OpenHandCursor)
            event.accept()
            return
        super().keyPressEvent(event)

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Space:
            self.is_space_panning = False
            self.unsetCursor()
            event.accept()
            return
        super().keyReleaseEvent(event)

    def mousePressEvent(self, event: QMouseEvent):
        if self.is_space_panning or event.button() == Qt.MouseButton.MiddleButton:
            self.pan_start_pos = event.position()
            self.setCursor(Qt.CursorShape.ClosedHandCursor)
            event.accept()
            return

        img_pos = self.mapToScene(event.position().toPoint())
        if self.active_tool:
            self.active_tool.mouse_press(img_pos, event)
            self.viewport().update()

        super().mousePressEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent):
        if self.pan_start_pos is not None:
            delta = event.position() - self.pan_start_pos
            self.pan_start_pos = event.position()
            self.horizontalScrollBar().setValue(int(self.horizontalScrollBar().value() - delta.x()))
            self.verticalScrollBar().setValue(int(self.verticalScrollBar().value() - delta.y()))
            event.accept()
            return

        img_pos = self.mapToScene(event.position().toPoint())
        self.hover_img_pos = img_pos

        # Emit pixel RGB signal under cursor
        pixel_rgb = (0, 0, 0)
        if self.document and self.document.original_image is not None:
            x, y = int(img_pos.x()), int(img_pos.y())
            if 0 <= x < self.document.width() and 0 <= y < self.document.height():
                pixel_rgb = tuple(self.document.original_image[y, x][:3])
        self.mouse_moved_signal.emit(img_pos, pixel_rgb)

        if self.active_tool:
            self.active_tool.mouse_move(img_pos, event)
            self.viewport().update()

        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event: QMouseEvent):
        if self.pan_start_pos is not None:
            self.pan_start_pos = None
            if self.is_space_panning:
                self.setCursor(Qt.CursorShape.OpenHandCursor)
            else:
                self.unsetCursor()
            event.accept()
            return

        img_pos = self.mapToScene(event.position().toPoint())
        if self.active_tool:
            self.active_tool.mouse_release(img_pos, event)
            self.viewport().update()

        super().mouseReleaseEvent(event)

    def drawForeground(self, painter: QPainter, rect: QRectF):
        """Renders tool overlay previews over canvas."""
        super().drawForeground(painter, rect)
        if self.active_tool:
            self.active_tool.draw_overlay(painter)

    # Drag & Drop Support
    def dragEnterEvent(self, event: QDragEnterEvent):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent):
        for url in event.mimeData().urls():
            file_path = url.toLocalFile()
            if file_path:
                self.image_dropped_signal.emit(file_path)
                break
