from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QMouseEvent
from app.tools.base_tool import BaseTool

class SelectMoveTool(BaseTool):
    """
    Select & Move tool for dragging canvas or adjusting position of layers/foreground.
    """

    def __init__(self, canvas):
        super().__init__(canvas)
        self.is_dragging = False
        self.last_pos = None

    def mouse_press(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_dragging = True
            self.last_pos = event.position()

    def mouse_move(self, img_pos: QPointF, event: QMouseEvent):
        if self.is_dragging and self.last_pos is not None:
            delta = event.position() - self.last_pos
            self.last_pos = event.position()
            # Pan canvas viewport
            hs = self.canvas.horizontalScrollBar()
            vs = self.canvas.verticalScrollBar()
            hs.setValue(int(hs.value() - delta.x()))
            vs.setValue(int(vs.value() - delta.y()))

    def mouse_release(self, img_pos: QPointF, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_dragging = False
            self.last_pos = None
