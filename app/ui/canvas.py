import math
from typing import Optional, List, Tuple
from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QPainter, QPixmap, QWheelEvent, QMouseEvent, QDragEnterEvent, QDropEvent, QPen, QColor, QBrush, QPolygonF
from PySide6.QtWidgets import QGraphicsView, QGraphicsScene, QGraphicsPixmapItem
from app.core.image_document import ImageDocument
from app.core.layer import Layer
from app.processing.compositing import composite_document
from app.utils.image_utils import numpy_to_qpixmap, load_image
from app.utils.logger import logger

class CanvasView(QGraphicsView):
    """
    Interactive multi-layer canvas supporting viewport zoom/pan,
    multi-layer hit selection, transform bounding box handles (8 resize + 1 rotation),
    smart snapping guides, and drag-and-drop multi-image import.
    """
    mouse_moved_signal = Signal(QPointF, tuple) # (img_pos, pixel_rgb)
    image_dropped_signal = Signal(str)
    images_dropped_signal = Signal(list)
    zoom_changed_signal = Signal(float)

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

        # Interactive Transform State
        self.drag_mode = None  # None, "move", "rotate", or handle name ("tl", "tc", "tr", "ml", "mr", "bl", "bc", "br")
        self.drag_start_canvas_pos: QPointF = None
        self.drag_start_layer_states = {} # {layer_id: (offset_x, offset_y, scale_x, scale_y, rotation)}
        self.active_handle = None

        self.setRenderHints(QPainter.RenderHint.Antialiasing | QPainter.RenderHint.SmoothPixmapTransform)
        self.setViewportUpdateMode(QGraphicsView.ViewportUpdateMode.FullViewportUpdate)
        self.setMouseTracking(True)
        self.setAcceptDrops(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.setTransformationAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)
        self.setResizeAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)

    def set_document(self, doc: ImageDocument):
        self.document = doc
        if self.document:
            self.document.add_change_listener(self.update_canvas)
        self.update_canvas()
        self.fit_in_view()

    def set_active_tool(self, tool):
        self.active_tool = tool
        self.viewport().update()

    def update_canvas(self, fast_drag: bool = False):
        """Renders composite multi-layer scene and updates canvas scene Rect."""
        if not self.document:
            self.scene.clear()
            self.pixmap_item = None
            return

        comp_rgba = composite_document(self.document, preview_mode=True, fast_drag=fast_drag)
        pixmap = numpy_to_qpixmap(comp_rgba)

        if self.pixmap_item is None:
            self.scene.clear()
            self.pixmap_item = self.scene.addPixmap(pixmap)
        else:
            self.pixmap_item.setPixmap(pixmap)

        self.scene.setSceneRect(0, 0, self.document.canvas_width, self.document.canvas_height)
        self.viewport().update()

    def fit_in_view(self):
        """Fits current document canvas to view viewport."""
        if self.scene and not self.scene.sceneRect().isEmpty():
            self.fitInView(self.scene.sceneRect(), Qt.AspectRatioMode.KeepAspectRatio)
            self.zoom_factor = self.transform().m11()
            self.zoom_changed_signal.emit(self.zoom_factor)

    def set_zoom_level(self, factor: float):
        """Sets explicit zoom scale factor."""
        self.resetTransform()
        self.scale(factor, factor)
        self.zoom_factor = factor
        self.zoom_changed_signal.emit(self.zoom_factor)

    def zoom_in(self):
        """Zoom in by 20%."""
        self.apply_zoom_step(1.2)

    def zoom_out(self):
        """Zoom out by 20%."""
        self.apply_zoom_step(1.0 / 1.2)

    def apply_zoom_step(self, factor: float):
        new_zoom = self.zoom_factor * factor
        if 0.02 <= new_zoom <= 50.0:
            self.scale(factor, factor)
            self.zoom_factor = self.transform().m11()
            self.zoom_changed_signal.emit(self.zoom_factor)

    def wheelEvent(self, event: QWheelEvent):
        """Zoom in/out smoothly anchored under the mouse cursor via wheel scroll."""
        zoom_in_factor = 1.15
        zoom_out_factor = 1.0 / zoom_in_factor

        if event.angleDelta().y() > 0:
            factor = zoom_in_factor
        else:
            factor = zoom_out_factor

        self.apply_zoom_step(factor)
        event.accept()

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

    # Multi-Layer Selection & Hit Testing
    def get_layer_at_point(self, scene_pos: QPointF) -> Optional[Layer]:
        """Performs hit testing on canvas to select topmost visible, un-locked layer under cursor."""
        if not self.document:
            return None

        px, py = scene_pos.x(), scene_pos.y()
        for lyr in reversed(self.document.layers):
            if not lyr.visible or lyr.locked:
                continue

            # Calculate layer rotated bounding box polygon
            poly, _, _ = self.get_layer_screen_polygon(lyr)
            if poly.containsPoint(scene_pos, Qt.FillRule.OddEvenFill):
                return lyr
        return None

    def get_layer_screen_polygon(self, lyr: Layer) -> Tuple[QPolygonF, QPointF, dict]:
        """Calculates rotated bounding box polygon and handle locations for a layer."""
        w, h = lyr.width(), lyr.height()
        scaled_w = w * lyr.scale_x
        scaled_h = h * lyr.scale_y

        cx = lyr.offset_x + scaled_w / 2.0
        cy = lyr.offset_y + scaled_h / 2.0

        rad = math.radians(lyr.rotation)
        cos_a, sin_a = math.cos(rad), math.sin(rad)

        def rotate_pt(x, y):
            dx = x - cx
            dy = y - cy
            rx = cx + dx * cos_a - dy * sin_a
            ry = cy + dx * sin_a + dy * cos_a
            return QPointF(rx, ry)

        tl = rotate_pt(lyr.offset_x, lyr.offset_y)
        tr = rotate_pt(lyr.offset_x + scaled_w, lyr.offset_y)
        br = rotate_pt(lyr.offset_x + scaled_w, lyr.offset_y + scaled_h)
        bl = rotate_pt(lyr.offset_x, lyr.offset_y + scaled_h)

        tc = rotate_pt(lyr.offset_x + scaled_w / 2.0, lyr.offset_y)
        bc = rotate_pt(lyr.offset_x + scaled_w / 2.0, lyr.offset_y + scaled_h)
        ml = rotate_pt(lyr.offset_x, lyr.offset_y + scaled_h / 2.0)
        mr = rotate_pt(lyr.offset_x + scaled_w, lyr.offset_y + scaled_h / 2.0)

        # Rotation handle offset 25px above top center
        rot_handle = rotate_pt(lyr.offset_x + scaled_w / 2.0, lyr.offset_y - 25.0)

        poly = QPolygonF([tl, tr, br, bl])
        handles = {
            "tl": tl, "tc": tc, "tr": tr,
            "ml": ml, "mr": mr,
            "bl": bl, "bc": bc, "br": br,
            "rot": rot_handle
        }
        return poly, QPointF(cx, cy), handles

    def get_handle_at_point(self, scene_pos: QPointF) -> Optional[str]:
        """Checks if cursor is over any bounding box handle of active layer."""
        if not self.document or not self.document.active_layer:
            return None

        lyr = self.document.active_layer
        if lyr.locked:
            return None

        _, _, handles = self.get_layer_screen_polygon(lyr)
        handle_size = 10.0 / self.zoom_factor

        for hname, hpos in handles.items():
            rect = QRectF(hpos.x() - handle_size / 2.0, hpos.y() - handle_size / 2.0, handle_size, handle_size)
            if rect.contains(scene_pos):
                return hname
        return None

    # Mouse Interaction
    def mousePressEvent(self, event: QMouseEvent):
        if self.is_space_panning or event.button() == Qt.MouseButton.MiddleButton:
            self.pan_start_pos = event.position()
            self.setCursor(Qt.CursorShape.ClosedHandCursor)
            event.accept()
            return

        scene_pos = self.mapToScene(event.position().toPoint())

        # Check handle click first
        handle = self.get_handle_at_point(scene_pos)
        if handle and self.document and self.document.active_layer:
            self.drag_mode = handle
            self.drag_start_canvas_pos = scene_pos
            self.drag_start_layer_states = {}
            for lyr in self.document.active_layers:
                self.drag_start_layer_states[lyr.id] = (lyr.offset_x, lyr.offset_y, lyr.scale_x, lyr.scale_y, lyr.rotation)
            event.accept()
            return

        # Direct Object Selection on Canvas
        clicked_layer = self.get_layer_at_point(scene_pos)
        if clicked_layer:
            multi = (event.modifiers() & Qt.KeyboardModifier.ControlModifier) or (event.modifiers() & Qt.KeyboardModifier.ShiftModifier)
            self.document.select_layer(clicked_layer.id, multi_select=bool(multi))
            if not clicked_layer.locked:
                self.drag_mode = "move"
                self.drag_start_canvas_pos = scene_pos
                self.drag_start_layer_states = {}
                for lyr in self.document.active_layers:
                    self.drag_start_layer_states[lyr.id] = (lyr.offset_x, lyr.offset_y, lyr.scale_x, lyr.scale_y, lyr.rotation)
        else:
            if not (event.modifiers() & Qt.KeyboardModifier.ControlModifier):
                pass  # Keep selection or active tool

        if self.active_tool and self.drag_mode is None:
            self.active_tool.mouse_press(scene_pos, event)
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

        scene_pos = self.mapToScene(event.position().toPoint())
        self.hover_img_pos = scene_pos

        # Update Mouse Cursor based on handle hover
        if self.drag_mode is None:
            handle = self.get_handle_at_point(scene_pos)
            if handle == "rot":
                self.setCursor(Qt.CursorShape.PointingHandCursor)
            elif handle in ["tl", "br"]:
                self.setCursor(Qt.CursorShape.SizeFDiagCursor)
            elif handle in ["tr", "bl"]:
                self.setCursor(Qt.CursorShape.SizeBDiagCursor)
            elif handle in ["tc", "bc"]:
                self.setCursor(Qt.CursorShape.SizeVerCursor)
            elif handle in ["ml", "mr"]:
                self.setCursor(Qt.CursorShape.SizeHorCursor)
            else:
                self.unsetCursor()

        # Handle Fast Dragging (Move / Scale / Rotate)
        if self.drag_mode and self.drag_start_canvas_pos and self.document and self.document.active_layer:
            dx = scene_pos.x() - self.drag_start_canvas_pos.x()
            dy = scene_pos.y() - self.drag_start_canvas_pos.y()
            lyr = self.document.active_layer

            if self.drag_mode == "move":
                for active_lyr in self.document.active_layers:
                    if not active_lyr.locked and active_lyr.id in self.drag_start_layer_states:
                        init_x, init_y, _, _, _ = self.drag_start_layer_states[active_lyr.id]
                        active_lyr.offset_x = init_x + dx
                        active_lyr.offset_y = init_y + dy

            elif self.drag_mode == "rot":
                # Rotation around center
                init_x, init_y, _, _, init_rot = self.drag_start_layer_states[lyr.id]
                cx = init_x + (lyr.width() * lyr.scale_x) / 2.0
                cy = init_y + (lyr.height() * lyr.scale_y) / 2.0

                angle = math.atan2(scene_pos.y() - cy, scene_pos.x() - cx)
                start_angle = math.atan2(self.drag_start_canvas_pos.y() - cy, self.drag_start_canvas_pos.x() - cx)
                delta_deg = math.degrees(angle - start_angle)
                lyr.rotation = (init_rot + delta_deg) % 360.0

            elif self.drag_mode in ["tl", "tr", "bl", "br", "tc", "bc", "ml", "mr"]:
                # Interactive Scaling Handle
                init_x, init_y, init_sx, init_sy, _ = self.drag_start_layer_states[lyr.id]
                orig_w, orig_h = lyr.width(), lyr.height()

                if "r" in self.drag_mode:
                    new_w = max(10.0, (orig_w * init_sx) + dx)
                    lyr.scale_x = new_w / float(orig_w)
                elif "l" in self.drag_mode:
                    new_w = max(10.0, (orig_w * init_sx) - dx)
                    lyr.scale_x = new_w / float(orig_w)
                    lyr.offset_x = init_x + dx

                if "b" in self.drag_mode:
                    new_h = max(10.0, (orig_h * init_sy) + dy)
                    lyr.scale_y = new_h / float(orig_h)
                elif "t" in self.drag_mode:
                    new_h = max(10.0, (orig_h * init_sy) - dy)
                    lyr.scale_y = new_h / float(orig_h)
                    lyr.offset_y = init_y + dy

                if lyr.lock_aspect:
                    avg_scale = (lyr.scale_x + lyr.scale_y) / 2.0
                    lyr.scale_x = avg_scale
                    lyr.scale_y = avg_scale

            # Update Canvas viewport at 60+ FPS without blocking side panels during drag
            self.update_canvas(fast_drag=True)
            event.accept()
            return

        # Emit cursor RGB
        pixel_rgb = (0, 0, 0)
        self.mouse_moved_signal.emit(scene_pos, pixel_rgb)

        if self.active_tool and self.drag_mode is None:
            self.active_tool.mouse_move(scene_pos, event)
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

        if self.drag_mode:
            self.drag_mode = None
            self.drag_start_canvas_pos = None
            self.drag_start_layer_states = {}
            if self.document:
                self.document.notify_changed()
            event.accept()
            return


        scene_pos = self.mapToScene(event.position().toPoint())
        if self.active_tool:
            self.active_tool.mouse_release(scene_pos, event)
            self.viewport().update()

        super().mouseReleaseEvent(event)

    def drawForeground(self, painter: QPainter, rect: QRectF):
        """Renders prominent canvas boundary frame, grid, layer handles, and tool overlays."""
        super().drawForeground(painter, rect)

        if not self.document:
            return

        cw = self.document.canvas_width
        ch = self.document.canvas_height

        # 1. Render Canvas Outer Boundary Frame (Neon Cyan + Amber Gold Corner Brackets)
        # Main Cyan Outline Frame (2px sharp border)
        frame_pen = QPen(QColor(0, 210, 255), 2.0 / self.zoom_factor)
        painter.setPen(frame_pen)
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawRect(QRectF(0, 0, cw, ch))

        # Outer Shadow Line around Canvas Frame
        outer_pen = QPen(QColor(0, 0, 0, 160), 1.0 / self.zoom_factor)
        painter.setPen(outer_pen)
        painter.drawRect(QRectF(-1.0 / self.zoom_factor, -1.0 / self.zoom_factor, cw + 2.0 / self.zoom_factor, ch + 2.0 / self.zoom_factor))

        # 4 Amber Gold Corner Brackets (L-shaped prominent corners)
        bracket_len = min(40.0, max(12.0, 24.0 / self.zoom_factor))
        bracket_pen = QPen(QColor(255, 170, 0), max(1.5, 3.0 / self.zoom_factor))
        painter.setPen(bracket_pen)

        # Top-Left Corner Bracket
        painter.drawLine(QPointF(0, 0), QPointF(bracket_len, 0))
        painter.drawLine(QPointF(0, 0), QPointF(0, bracket_len))

        # Top-Right Corner Bracket
        painter.drawLine(QPointF(cw, 0), QPointF(cw - bracket_len, 0))
        painter.drawLine(QPointF(cw, 0), QPointF(cw, bracket_len))

        # Bottom-Right Corner Bracket
        painter.drawLine(QPointF(cw, ch), QPointF(cw - bracket_len, ch))
        painter.drawLine(QPointF(cw, ch), QPointF(cw, ch - bracket_len))

        # Bottom-Left Corner Bracket
        painter.drawLine(QPointF(0, ch), QPointF(bracket_len, ch))
        painter.drawLine(QPointF(0, ch), QPointF(0, ch - bracket_len))

        # Floating Dimension Tag Badge Above Top-Left Corner
        badge_text = f" Canvas: {cw} × {ch} px "
        badge_font = painter.font()
        badge_font.setPointPointSize(10)
        badge_font.setBold(True)
        painter.setFont(badge_font)

        metrics = painter.fontMetrics()
        text_w = metrics.horizontalAdvance(badge_text) + 12
        text_h = metrics.height() + 6

        # Position tag above canvas
        badge_rect = QRectF(0, -text_h - (6.0 / self.zoom_factor), text_w, text_h)

        # Draw rounded badge background with semi-transparent cyan glow
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(QColor(0, 120, 215, 230)))
        painter.drawRoundedRect(badge_rect, 4, 4)

        painter.setPen(QPen(QColor(255, 255, 255)))
        painter.drawText(badge_rect, Qt.AlignmentFlag.AlignCenter, badge_text)

        # 2. Render Grid if enabled
        if self.document.show_grid:
            grid_size = self.document.grid_size
            pen = QPen(QColor(255, 255, 255, int(self.document.grid_opacity * 255)))
            pen.setStyle(Qt.PenStyle.DotLine)
            painter.setPen(pen)
            for x in range(0, cw, grid_size):
                painter.drawLine(x, 0, x, ch)
            for y in range(0, ch, grid_size):
                painter.drawLine(0, y, cw, y)

        # 3. Render Selection Bounding Box & Handles for active layer(s)
        if self.document.active_layers:
            for lyr in self.document.active_layers:
                poly, center, handles = self.get_layer_screen_polygon(lyr)

                # Draw Bounding Box Outline
                pen = QPen(QColor(0, 120, 215), 1.5 / self.zoom_factor)
                pen.setStyle(Qt.PenStyle.DashLine if lyr.locked else Qt.PenStyle.SolidLine)
                painter.setPen(pen)
                painter.setBrush(Qt.BrushStyle.NoBrush)
                painter.drawPolygon(poly)

                if not lyr.locked:
                    # Draw Line to Rotation Handle
                    painter.setPen(QPen(QColor(0, 120, 215), 1.0 / self.zoom_factor))
                    painter.drawLine(handles["tc"], handles["rot"])

                    # Draw Handles
                    handle_sz = 8.0 / self.zoom_factor
                    painter.setPen(QPen(QColor(255, 255, 255), 1.0 / self.zoom_factor))
                    painter.setBrush(QBrush(QColor(0, 120, 215)))

                    for hname, hpos in handles.items():
                        if hname == "rot":
                            painter.setBrush(QBrush(QColor(255, 140, 0)))
                            painter.drawEllipse(hpos, handle_sz / 1.5, handle_sz / 1.5)
                            painter.setBrush(QBrush(QColor(0, 120, 215)))
                        else:
                            hrect = QRectF(hpos.x() - handle_sz / 2.0, hpos.y() - handle_sz / 2.0, handle_sz, handle_sz)
                            painter.drawRect(hrect)

        # 4. Active Tool Overlay
        if self.active_tool:
            self.active_tool.draw_overlay(painter)


    # Drag & Drop Multiple Image Support
    def dragEnterEvent(self, event: QDragEnterEvent):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event: QDropEvent):
        file_paths = []
        for url in event.mimeData().urls():
            file_path = url.toLocalFile()
            if file_path:
                file_paths.append(file_path)

        if len(file_paths) == 1:
            self.image_dropped_signal.emit(file_paths[0])
        elif len(file_paths) > 1:
            self.images_dropped_signal.emit(file_paths)

