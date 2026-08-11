from PySide6.QtCore import Qt, Signal, QSize
from PySide6.QtWidgets import QWidget, QVBoxLayout, QPushButton, QButtonGroup, QFrame, QToolTip

class ToolBarPanel(QFrame):
    """Left sidebar tool selection panel with icon buttons and tooltips."""
    tool_changed_signal = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedWidth(54)
        self.setObjectName("toolbar_panel")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(4, 8, 4, 8)
        layout.setSpacing(6)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        self.btn_group = QButtonGroup(self)
        self.btn_group.setExclusive(True)

        tools = [
            ("Select", "Select & Pan Tool (H / Space)", "✋"),
            ("Brush", "Restore Brush Tool (B)", "🖌️"),
            ("Eraser", "Eraser Tool (E)", "🧹"),
            ("MagicWand", "Magic Wand Color Select (W)", "🪄"),
            ("Lasso", "Lasso Selection Tool (L)", "✂️"),
            ("Crop", "Crop Canvas Tool (C)", "🖼️")
        ]

        self.buttons = {}
        for tool_id, tooltip, icon_str in tools:
            btn = QPushButton(icon_str, self)
            btn.setObjectName("btn_tool")
            btn.setCheckable(True)
            btn.setToolTip(tooltip)
            btn.setFixedSize(QSize(44, 44))

            self.btn_group.addButton(btn)
            self.buttons[tool_id] = btn
            layout.addWidget(btn)

            btn.clicked.connect(lambda checked, tid=tool_id: self.tool_changed_signal.emit(tid))

        # Default select tool
        self.buttons["Select"].setChecked(True)

    def set_active_tool(self, tool_id: str):
        if tool_id in self.buttons:
            self.buttons[tool_id].setChecked(True)
