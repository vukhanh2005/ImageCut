from PySide6.QtCore import Qt, Signal, QSize
from PySide6.QtWidgets import QWidget, QHBoxLayout, QPushButton, QLabel, QFrame, QProgressBar

class TopBarPanel(QFrame):
    """Header tool bar with primary quick actions and AI execution status."""
    open_signal = Signal()
    save_signal = Signal()
    undo_signal = Signal()
    redo_signal = Signal()
    auto_remove_signal = Signal()
    batch_signal = Signal()
    export_signal = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedHeight(56)
        self.setObjectName("top_bar")

        layout = QHBoxLayout(self)
        layout.setContentsMargins(12, 6, 12, 6)
        layout.setSpacing(10)

        # File actions
        self.btn_open = QPushButton("📁 Open Image", self)
        self.btn_open.setToolTip("Open Image File (Ctrl+O)")
        self.btn_open.clicked.connect(self.open_signal.emit)
        layout.addWidget(self.btn_open)

        self.btn_undo = QPushButton("↩️ Undo", self)
        self.btn_undo.setToolTip("Undo Last Action (Ctrl+Z)")
        self.btn_undo.clicked.connect(self.undo_signal.emit)
        layout.addWidget(self.btn_undo)

        self.btn_redo = QPushButton("↪️ Redo", self)
        self.btn_redo.setToolTip("Redo (Ctrl+Shift+Z)")
        self.btn_redo.clicked.connect(self.redo_signal.emit)
        layout.addWidget(self.btn_redo)

        # Separator
        sep = QFrame(self)
        sep.setFrameShape(QFrame.Shape.VLine)
        sep.setStyleSheet("color: #2D3748;")
        layout.addWidget(sep)

        # AI Action Button (Primary Highlighted)
        self.btn_auto_remove = QPushButton("✨ Auto Remove BG", self)
        self.btn_auto_remove.setObjectName("btn_primary")
        self.btn_auto_remove.setToolTip("Run AI Model to Remove Background")
        self.btn_auto_remove.setFixedHeight(38)
        self.btn_auto_remove.clicked.connect(self.auto_remove_signal.emit)
        layout.addWidget(self.btn_auto_remove)

        self.progress_bar = QProgressBar(self)
        self.progress_bar.setFixedWidth(140)
        self.progress_bar.setVisible(False)
        self.progress_bar.setStyleSheet("""
            QProgressBar {
                border: 1px solid #3B4252;
                border-radius: 4px;
                text-align: center;
                background-color: #1E202E;
                color: white;
            }
            QProgressBar::chunk {
                background-color: #6C5CE7;
                border-radius: 4px;
            }
        """)
        layout.addWidget(self.progress_bar)

        layout.addStretch()

        # Batch & Export
        self.btn_batch = QPushButton("⚡ Batch Removal", self)
        self.btn_batch.setToolTip("Process Folder of Multiple Images")
        self.btn_batch.clicked.connect(self.batch_signal.emit)
        layout.addWidget(self.btn_batch)

        self.btn_export = QPushButton("💾 Export Image", self)
        self.btn_export.setToolTip("Export Result Image (Ctrl+E)")
        self.btn_export.clicked.connect(self.export_signal.emit)
        layout.addWidget(self.btn_export)

    def show_progress(self, val: int):
        self.progress_bar.setValue(val)
        self.progress_bar.setVisible(True)

    def hide_progress(self):
        self.progress_bar.setVisible(False)
