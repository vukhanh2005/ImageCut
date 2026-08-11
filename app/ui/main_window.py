import os
from PySide6.QtCore import Qt, QPointF
from PySide6.QtGui import QAction, QKeySequence, QIcon
from PySide6.QtWidgets import (QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
                             QTabWidget, QFileDialog, QMessageBox, QLabel, QStatusBar)

from app.core.image_document import ImageDocument
from app.core.project import ProjectManager
from app.ui.top_bar import TopBarPanel
from app.ui.toolbar import ToolBarPanel
from app.ui.canvas import CanvasView
from app.ui.panels.background_panel import BackgroundPanel
from app.ui.panels.mask_panel import MaskPanel
from app.ui.panels.image_panel import ImagePanel
from app.ui.panels.transform_panel import TransformPanel
from app.ui.dialogs.export_dialog import ExportDialog
from app.ui.dialogs.batch_dialog import BatchDialog
from app.ui.dialogs.settings_dialog import SettingsDialog

from app.tools.brush_tool import MaskBrushTool
from app.tools.magic_wand_tool import MagicWandTool
from app.tools.lasso_tool import LassoTool
from app.tools.crop_tool import CropTool
from app.tools.select_tool import SelectMoveTool

from app.workers.inference_worker import BackgroundRemovalWorker
from app.utils.image_utils import load_image
from app.utils.settings import settings
from app.utils.logger import logger

class MainWindow(QMainWindow):
    """
    Main Application Window for Personal Background Remover & Image Editor.
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Personal Background Remover & Image Editor")
        self.resize(1360, 860)

        self.document = ImageDocument()

        # UI Setup
        self._init_menu_bar()
        self._init_ui_layout()
        self._init_tools()
        self._init_shortcuts()

        # Connect Document Change Events
        self.document.add_change_listener(self._on_document_changed)
        self.canvas.image_dropped_signal.connect(self.open_image_file)

    def _init_menu_bar(self):
        menubar = self.menuBar()

        # File Menu
        menu_file = menubar.addMenu("&File")

        act_open = QAction("&Open Image...", self)
        act_open.setShortcut("Ctrl+O")
        act_open.triggered.connect(self.action_open)
        menu_file.addAction(act_open)

        act_open_proj = QAction("Open Project (.bgrem)...", self)
        act_open_proj.triggered.connect(self.action_open_project)
        menu_file.addAction(act_open_proj)

        act_save_proj = QAction("&Save Project", self)
        act_save_proj.setShortcut("Ctrl+S")
        act_save_proj.triggered.connect(self.action_save_project)
        menu_file.addAction(act_save_proj)

        menu_file.addSeparator()

        act_export = QAction("&Export Image...", self)
        act_export.setShortcut("Ctrl+E")
        act_export.triggered.connect(self.action_export)
        menu_file.addAction(act_export)

        menu_file.addSeparator()
        act_exit = QAction("E&xit", self)
        act_exit.triggered.connect(self.close)
        menu_file.addAction(act_exit)

        # Edit Menu
        menu_edit = menubar.addMenu("&Edit")

        self.act_undo = QAction("&Undo", self)
        self.act_undo.setShortcut("Ctrl+Z")
        self.act_undo.triggered.connect(self.action_undo)
        menu_edit.addAction(self.act_undo)

        self.act_redo = QAction("&Redo", self)
        self.act_redo.setShortcut("Ctrl+Shift+Z")
        self.act_redo.triggered.connect(self.action_redo)
        menu_edit.addAction(self.act_redo)

        # Tools Menu
        menu_tools = menubar.addMenu("&Tools")
        act_remove_bg = QAction("✨ Auto Remove Background", self)
        act_remove_bg.triggered.connect(self.action_auto_remove)
        menu_tools.addAction(act_remove_bg)

        act_batch = QAction("⚡ Batch Background Removal...", self)
        act_batch.triggered.connect(self.action_batch)
        menu_tools.addAction(act_batch)

        # Settings Menu
        menu_settings = menubar.addMenu("&Settings")
        act_pref = QAction("Preferences...", self)
        act_pref.triggered.connect(self.action_settings)
        menu_settings.addAction(act_pref)

    def _init_ui_layout(self):
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)

        main_vbox = QVBoxLayout(central_widget)
        main_vbox.setContentsMargins(0, 0, 0, 0)
        main_vbox.setSpacing(0)

        # Top Bar
        self.top_bar = TopBarPanel(self)
        self.top_bar.open_signal.connect(self.action_open)
        self.top_bar.undo_signal.connect(self.action_undo)
        self.top_bar.redo_signal.connect(self.action_redo)
        self.top_bar.auto_remove_signal.connect(self.action_auto_remove)
        self.top_bar.batch_signal.connect(self.action_batch)
        self.top_bar.export_signal.connect(self.action_export)
        main_vbox.addWidget(self.top_bar)

        # Content Area (Left Toolbar + Center Canvas + Right Control Tabs)
        content_hbox = QHBoxLayout()
        content_hbox.setContentsMargins(0, 0, 0, 0)
        content_hbox.setSpacing(0)

        # Left Toolbar
        self.toolbar_panel = ToolBarPanel(self)
        self.toolbar_panel.tool_changed_signal.connect(self.select_tool_by_name)
        content_hbox.addWidget(self.toolbar_panel)

        # Center Canvas
        self.canvas = CanvasView(self)
        self.canvas.mouse_moved_signal.connect(self._on_canvas_mouse_moved)
        content_hbox.addWidget(self.canvas, stretch=1)

        # Right Side Control Panel Tabs
        self.right_tabs = QTabWidget(self)
        self.right_tabs.setFixedWidth(320)

        self.panel_bg = BackgroundPanel(self)
        self.panel_mask = MaskPanel(self)
        self.panel_image = ImagePanel(self)
        self.panel_transform = TransformPanel(self)
        self.panel_transform.apply_crop_signal.connect(self.action_apply_crop)

        self.right_tabs.addTab(self.panel_bg, "Background")
        self.right_tabs.addTab(self.panel_mask, "Mask")
        self.right_tabs.addTab(self.panel_image, "Image")
        self.right_tabs.addTab(self.panel_transform, "Transform")

        content_hbox.addWidget(self.right_tabs)
        main_vbox.addLayout(content_hbox, stretch=1)

        # Bottom Status Bar
        self.statusbar = QStatusBar(self)
        self.setStatusBar(self.statusbar)

        self.lbl_status_msg = QLabel("Ready. Open an image to start.", self)
        self.lbl_status_dim = QLabel("Res: 0x0", self)
        self.lbl_status_zoom = QLabel("Zoom: 100%", self)
        self.lbl_status_color = QLabel("RGB: -", self)

        self.statusbar.addWidget(self.lbl_status_msg, stretch=1)
        self.statusbar.addPermanentWidget(self.lbl_status_color)
        self.statusbar.addPermanentWidget(self.lbl_status_dim)
        self.statusbar.addPermanentWidget(self.lbl_status_zoom)

    def _init_tools(self):
        """Instantiates canvas interaction tool objects."""
        self.tools = {
            "Select": SelectMoveTool(self.canvas),
            "Brush": MaskBrushTool(self.canvas, mode="Restore"),
            "Eraser": MaskBrushTool(self.canvas, mode="Eraser"),
            "MagicWand": MagicWandTool(self.canvas),
            "Lasso": LassoTool(self.canvas, mode="Remove"),
            "Crop": CropTool(self.canvas)
        }
        self.select_tool_by_name("Select")

    def _init_shortcuts(self):
        """Registers quick single-key keyboard shortcuts."""
        pass  # Qt handles standard QAction shortcuts

    def keyPressEvent(self, event):
        key = event.key()
        if key == Qt.Key.Key_B:
            self.toolbar_panel.set_active_tool("Brush")
            self.select_tool_by_name("Brush")
        elif key == Qt.Key.Key_E:
            self.toolbar_panel.set_active_tool("Eraser")
            self.select_tool_by_name("Eraser")
        elif key == Qt.Key.Key_W:
            self.toolbar_panel.set_active_tool("MagicWand")
            self.select_tool_by_name("MagicWand")
        elif key == Qt.Key.Key_L:
            self.toolbar_panel.set_active_tool("Lasso")
            self.select_tool_by_name("Lasso")
        elif key == Qt.Key.Key_C:
            self.toolbar_panel.set_active_tool("Crop")
            self.select_tool_by_name("Crop")
        elif key == Qt.Key.Key_H:
            self.toolbar_panel.set_active_tool("Select")
            self.select_tool_by_name("Select")
        elif event.modifiers() == Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_0:
            self.canvas.fit_in_view()
        elif event.modifiers() == Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_1:
            self.canvas.set_zoom_level(1.0)
        else:
            super().keyPressEvent(event)

    def select_tool_by_name(self, tool_name: str):
        if tool_name in self.tools:
            tool = self.tools[tool_name]
            self.canvas.set_active_tool(tool)
            self.lbl_status_msg.setText(f"Active Tool: {tool_name}")

    def open_image_file(self, file_path: str):
        """Loads an image into the document model."""
        try:
            arr = load_image(file_path)
            self.document.set_original_image(arr)
            self.canvas.set_document(self.document)
            self.panel_bg.set_document(self.document)
            self.panel_mask.set_document(self.document)
            self.panel_image.set_document(self.document)
            self.panel_transform.set_document(self.document)

            self.lbl_status_dim.setText(f"Res: {self.document.width()}x{self.document.height()}")
            self.lbl_status_msg.setText(f"Loaded: {os.path.basename(file_path)}")
        except Exception as e:
            logger.error(f"Error opening image {file_path}: {e}", exc_info=True)
            QMessageBox.critical(self, "Error Opening Image", f"Could not load image file:\n{e}")

    # Actions
    def action_open(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Open Image", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)")
        if file_path:
            self.open_image_file(file_path)

    def action_open_project(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Open Project", "", "Project Files (*.bgrem)")
        if file_path:
            try:
                doc = ProjectManager.load_project(file_path)
                self.document = doc
                self.canvas.set_document(self.document)
                self.panel_bg.set_document(self.document)
                self.panel_mask.set_document(self.document)
                self.panel_image.set_document(self.document)
                self.panel_transform.set_document(self.document)
                self.lbl_status_msg.setText(f"Loaded Project: {os.path.basename(file_path)}")
            except Exception as e:
                QMessageBox.critical(self, "Project Load Error", f"Could not load project:\n{e}")

    def action_save_project(self):
        if self.document.original_image is None:
            QMessageBox.warning(self, "No Image", "Please open an image before saving a project.")
            return

        file_path, _ = QFileDialog.getSaveFileName(self, "Save Project", "project.bgrem", "Project Files (*.bgrem)")
        if file_path:
            success = ProjectManager.save_project(self.document, file_path)
            if success:
                self.lbl_status_msg.setText(f"Project saved: {os.path.basename(file_path)}")
            else:
                QMessageBox.critical(self, "Error", "Failed to save project.")

    def action_undo(self):
        if self.document and self.document.undo_stack.can_undo():
            self.document.undo_stack.undo()

    def action_redo(self):
        if self.document and self.document.undo_stack.can_redo():
            self.document.undo_stack.redo()

    def action_auto_remove(self):
        if self.document.original_image is None:
            QMessageBox.warning(self, "No Image", "Please open an image first.")
            return

        self.top_bar.show_progress(10)
        self.top_bar.btn_auto_remove.setEnabled(False)

        model_name = settings.get("ai_model", "RMBG-1.4")
        device = settings.get("ai_device", "Auto")

        self.worker = BackgroundRemovalWorker(self.document.original_image, engine_type="AI", model_name=model_name, device=device)
        self.worker.progress.connect(self.top_bar.show_progress)
        self.worker.finished.connect(self._on_ai_finished)
        self.worker.error.connect(self._on_ai_error)
        self.worker.start()

    def _on_ai_finished(self, mask):
        self.top_bar.hide_progress()
        self.top_bar.btn_auto_remove.setEnabled(True)
        self.document.update_mask(mask, description="AI Background Removal")
        self.lbl_status_msg.setText("AI Background Removal complete!")

    def _on_ai_error(self, err_msg: str):
        self.top_bar.hide_progress()
        self.top_bar.btn_auto_remove.setEnabled(True)
        QMessageBox.critical(self, "Background Removal Error", f"Failed to run background removal:\n{err_msg}")

    def action_apply_crop(self):
        crop_tool = self.tools.get("Crop")
        if crop_tool and isinstance(crop_tool, CropTool):
            crop_tool.apply_crop()

    def action_batch(self):
        dlg = BatchDialog(self)
        dlg.exec()

    def action_export(self):
        if self.document.original_image is None:
            QMessageBox.warning(self, "No Image", "Please open an image before exporting.")
            return
        dlg = ExportDialog(self.document, self)
        dlg.exec()

    def action_settings(self):
        dlg = SettingsDialog(self)
        if dlg.exec():
            # Refresh theme or model preferences if updated
            pass

    def _on_document_changed(self):
        self.act_undo.setEnabled(self.document.undo_stack.can_undo())
        self.act_redo.setEnabled(self.document.undo_stack.can_redo())
        self.canvas.update_canvas()

    def _on_canvas_mouse_moved(self, img_pos: QPointF, rgb_color: tuple):
        self.lbl_status_color.setText(f"RGB: {rgb_color}")
        self.lbl_status_zoom.setText(f"Zoom: {int(self.canvas.zoom_factor * 100)}%")
