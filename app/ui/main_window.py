import os
from typing import List
from PySide6.QtCore import Qt, QPointF
from PySide6.QtGui import QAction, QKeySequence, QIcon
from PySide6.QtWidgets import (QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
                             QTabWidget, QFileDialog, QMessageBox, QLabel, QStatusBar, QPushButton)


from app.core.image_document import ImageDocument
from app.core.layer import Layer
from app.core.project import ProjectManager
from app.ui.top_bar import TopBarPanel
from app.ui.toolbar import ToolBarPanel
from app.ui.canvas import CanvasView
from app.ui.panels.layer_panel import LayerManagerPanel
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
    Main Application Window for Multi-Layer Image Compositing & Editor.
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("ImageCut — Multi-Layer Image Compositing & Background Remover")
        self.resize(1400, 900)

        self.document = ImageDocument()
        self._copied_layers: List[Layer] = []

        # UI Setup
        self._init_menu_bar()
        self._init_ui_layout()
        self._init_tools()

        # Connect Document Change Events
        self.document.add_change_listener(self._on_document_changed)
        self.canvas.image_dropped_signal.connect(lambda path: self.import_image_files([path]))
        self.canvas.images_dropped_signal.connect(self.import_image_files)

    def _init_menu_bar(self):
        menubar = self.menuBar()

        # File Menu
        menu_file = menubar.addMenu("&File")

        act_import = QAction("📥 &Import Image(s)...", self)
        act_import.setShortcut("Ctrl+I")
        act_import.triggered.connect(self.action_import_images)
        menu_file.addAction(act_import)

        act_open = QAction("&Open Single Image...", self)
        act_open.setShortcut("Ctrl+O")
        act_open.triggered.connect(self.action_open)
        menu_file.addAction(act_open)

        menu_file.addSeparator()

        act_open_proj = QAction("Open Project (.bgrem)...", self)
        act_open_proj.triggered.connect(self.action_open_project)
        menu_file.addAction(act_open_proj)

        act_save_proj = QAction("&Save Project (.bgrem)", self)
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

        menu_edit.addSeparator()

        act_copy = QAction("Copy Layer(s)", self)
        act_copy.setShortcut("Ctrl+C")
        act_copy.triggered.connect(self.action_copy_layer)
        menu_edit.addAction(act_copy)

        act_paste = QAction("Paste Layer(s)", self)
        act_paste.setShortcut("Ctrl+V")
        act_paste.triggered.connect(self.action_paste_layer)
        menu_edit.addAction(act_paste)

        act_dup = QAction("Duplicate Layer(s)", self)
        act_dup.setShortcut("Ctrl+D")
        act_dup.triggered.connect(self.action_duplicate_layer)
        menu_edit.addAction(act_dup)

        act_del = QAction("Delete Selected Layer(s)", self)
        act_del.setShortcut("Delete")
        act_del.triggered.connect(self.action_delete_layer)
        menu_edit.addAction(act_del)

        # Layer Menu (Z-order & Grouping)
        menu_layer = menubar.addMenu("&Layer")

        act_top = QAction("Move to Top", self)
        act_top.setShortcut("Ctrl+Shift+]")
        act_top.triggered.connect(lambda: self.document.move_layer_top())
        menu_layer.addAction(act_top)

        act_up = QAction("Move Up", self)
        act_up.setShortcut("Ctrl+]")
        act_up.triggered.connect(lambda: self.document.move_layer_up())
        menu_layer.addAction(act_up)

        act_down = QAction("Move Down", self)
        act_down.setShortcut("Ctrl+[")
        act_down.triggered.connect(lambda: self.document.move_layer_down())
        menu_layer.addAction(act_down)

        act_bot = QAction("Move to Bottom", self)
        act_bot.setShortcut("Ctrl+Shift+[")
        act_bot.triggered.connect(lambda: self.document.move_layer_bottom())
        menu_layer.addAction(act_bot)

        menu_layer.addSeparator()

        act_group = QAction("Group Selected Layers", self)
        act_group.setShortcut("Ctrl+G")
        act_group.triggered.connect(lambda: self.document.group_layers(list(self.document.active_layer_ids)))
        menu_layer.addAction(act_group)

        act_add_text = QAction("➕ Add Text Layer", self)
        act_add_text.triggered.connect(self.action_add_text_layer)
        menu_layer.addAction(act_add_text)

        act_add_shape = QAction("➕ Add Shape Layer", self)
        act_add_shape.triggered.connect(self.action_add_shape_layer)
        menu_layer.addAction(act_add_shape)

        # View Menu
        menu_view = menubar.addMenu("&View")
        act_zoom_in = QAction("🔍 Zoom &In", self)
        act_zoom_in.setShortcuts([QKeySequence("Ctrl++"), QKeySequence("Ctrl+=")])
        act_zoom_in.triggered.connect(lambda: self.canvas.zoom_in())
        menu_view.addAction(act_zoom_in)

        act_zoom_out = QAction("🔍 Zoom &Out", self)
        act_zoom_out.setShortcut("Ctrl+-")
        act_zoom_out.triggered.connect(lambda: self.canvas.zoom_out())
        menu_view.addAction(act_zoom_out)

        menu_view.addSeparator()

        act_zoom_fit = QAction("🎯 Fit to Screen", self)
        act_zoom_fit.setShortcut("Ctrl+0")
        act_zoom_fit.triggered.connect(lambda: self.canvas.fit_in_view())
        menu_view.addAction(act_zoom_fit)

        act_zoom_100 = QAction("100% Actual Size", self)
        act_zoom_100.setShortcut("Ctrl+1")
        act_zoom_100.triggered.connect(lambda: self.canvas.set_zoom_level(1.0))
        menu_view.addAction(act_zoom_100)

        # Tools Menu
        menu_tools = menubar.addMenu("&Tools")
        act_remove_bg = QAction("✨ Auto Remove Background (Selected Layer)", self)
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
        self.top_bar.open_signal.connect(self.action_import_images)
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
        self.canvas.zoom_changed_signal.connect(self._on_zoom_changed)
        content_hbox.addWidget(self.canvas, stretch=1)

        # Right Side Control Panel Tabs
        self.right_tabs = QTabWidget(self)
        self.right_tabs.setFixedWidth(340)

        self.panel_layers = LayerManagerPanel(self)
        self.panel_layers.btn_add.clicked.connect(self.action_import_images)

        self.panel_transform = TransformPanel(self)
        self.panel_transform.apply_crop_signal.connect(self.action_apply_crop)

        self.panel_mask = MaskPanel(self)
        self.panel_image = ImagePanel(self)
        self.panel_bg = BackgroundPanel(self)

        self.right_tabs.addTab(self.panel_layers, "Layers")
        self.right_tabs.addTab(self.panel_transform, "Transform")
        self.right_tabs.addTab(self.panel_mask, "Mask")
        self.right_tabs.addTab(self.panel_image, "Image")
        self.right_tabs.addTab(self.panel_bg, "Background")

        content_hbox.addWidget(self.right_tabs)
        main_vbox.addLayout(content_hbox, stretch=1)

        # Bottom Status Bar with Zoom Controls
        self.statusbar = QStatusBar(self)
        self.setStatusBar(self.statusbar)

        self.lbl_status_msg = QLabel("Ready. Import images to start compositing.", self)
        self.lbl_status_dim = QLabel("Canvas: 1920x1080", self)
        self.lbl_status_color = QLabel("X: 0, Y: 0", self)

        # Status Bar Zoom Controls Widget
        zoom_bar = QWidget(self)
        zoom_layout = QHBoxLayout(zoom_bar)
        zoom_layout.setContentsMargins(0, 0, 0, 0)
        zoom_layout.setSpacing(4)

        btn_z_out = QPushButton("➖", self)
        btn_z_out.setFixedSize(24, 22)
        btn_z_out.setToolTip("Zoom Out (Ctrl+ - / Scroll Down)")
        btn_z_out.clicked.connect(lambda: self.canvas.zoom_out())

        self.lbl_status_zoom = QLabel("Zoom: 100%", self)
        self.lbl_status_zoom.setStyleSheet("font-weight: bold; padding: 0 4px;")

        btn_z_in = QPushButton("➕", self)
        btn_z_in.setFixedSize(24, 22)
        btn_z_in.setToolTip("Zoom In (Ctrl+ + / Scroll Up)")
        btn_z_in.clicked.connect(lambda: self.canvas.zoom_in())

        btn_z_fit = QPushButton("Fit 🔍", self)
        btn_z_fit.setFixedHeight(22)
        btn_z_fit.setToolTip("Fit Canvas to Window (Ctrl+0)")
        btn_z_fit.clicked.connect(lambda: self.canvas.fit_in_view())

        btn_z_100 = QPushButton("100% 🎯", self)
        btn_z_100.setFixedHeight(22)
        btn_z_100.setToolTip("100% Actual Size (Ctrl+1)")
        btn_z_100.clicked.connect(lambda: self.canvas.set_zoom_level(1.0))

        zoom_layout.addWidget(btn_z_out)
        zoom_layout.addWidget(self.lbl_status_zoom)
        zoom_layout.addWidget(btn_z_in)
        zoom_layout.addWidget(btn_z_fit)
        zoom_layout.addWidget(btn_z_100)

        self.statusbar.addWidget(self.lbl_status_msg, stretch=1)
        self.statusbar.addPermanentWidget(self.lbl_status_color)
        self.statusbar.addPermanentWidget(self.lbl_status_dim)
        self.statusbar.addPermanentWidget(zoom_bar)

        # Sync document with all panels
        self._bind_document_to_panels()


    def _bind_document_to_panels(self):
        self.canvas.set_document(self.document)
        self.panel_layers.set_document(self.document)
        self.panel_transform.set_document(self.document)
        self.panel_mask.set_document(self.document)
        self.panel_image.set_document(self.document)
        self.panel_bg.set_document(self.document)

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

    def keyPressEvent(self, event):
        key = event.key()
        modifiers = event.modifiers()

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
        elif modifiers == Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_0:
            self.canvas.fit_in_view()
        elif modifiers == Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_1:
            self.canvas.set_zoom_level(1.0)
        elif modifiers == Qt.KeyboardModifier.ControlModifier and key == Qt.Key.Key_D:
            self.action_duplicate_layer()
        elif key == Qt.Key.Key_Delete or key == Qt.Key.Key_Backspace:
            self.action_delete_layer()
        else:
            super().keyPressEvent(event)

    def select_tool_by_name(self, tool_name: str):
        if tool_name in self.tools:
            tool = self.tools[tool_name]
            self.canvas.set_active_tool(tool)
            self.lbl_status_msg.setText(f"Active Tool: {tool_name}")

    # Import Methods
    def import_image_files(self, file_paths: List[str]):
        """Imports multiple images as independent layers."""
        added_layers = []
        for path in file_paths:
            try:
                arr = load_image(path)
                layer = self.document.add_image_layer(arr, name=os.path.basename(path))
                added_layers.append(layer)
            except Exception as e:
                logger.error(f"Error importing image {path}: {e}", exc_info=True)

        if added_layers:
            self.canvas.fit_in_view()
            self.lbl_status_dim.setText(f"Canvas: {self.document.width()}x{self.document.height()}")
            self.lbl_status_msg.setText(f"Imported {len(added_layers)} layer(s)")

    def _on_zoom_changed(self, zoom_factor: float):
        self.lbl_status_zoom.setText(f"Zoom: {int(zoom_factor * 100)}%")


    def action_import_images(self):
        file_paths, _ = QFileDialog.getOpenFileNames(
            self, "Import Images", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)"
        )
        if file_paths:
            self.import_image_files(file_paths)

    def open_image_file(self, file_path: str):
        self.import_image_files([file_path])

    def action_open(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Open Image", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)")
        if file_path:
            self.import_image_files([file_path])

    # Project Save / Load
    def action_open_project(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Open Project", "", "Project Files (*.bgrem)")
        if file_path:
            try:
                doc = ProjectManager.load_project(file_path)
                self.document = doc
                self._bind_document_to_panels()
                self.lbl_status_msg.setText(f"Loaded Project: {os.path.basename(file_path)}")
            except Exception as e:
                QMessageBox.critical(self, "Project Load Error", f"Could not load project:\n{e}")

    def action_save_project(self):
        if not self.document.layers and self.document.original_image is None:
            QMessageBox.warning(self, "No Image", "Please import an image before saving a project.")
            return

        file_path, _ = QFileDialog.getSaveFileName(self, "Save Project", "project.bgrem", "Project Files (*.bgrem)")
        if file_path:
            success = ProjectManager.save_project(self.document, file_path)
            if success:
                self.lbl_status_msg.setText(f"Project saved: {os.path.basename(file_path)}")
            else:
                QMessageBox.critical(self, "Error", "Failed to save project.")

    # Edit Actions
    def action_undo(self):
        if self.document and self.document.undo_stack.can_undo():
            self.document.undo_stack.undo()

    def action_redo(self):
        if self.document and self.document.undo_stack.can_redo():
            self.document.undo_stack.redo()

    def action_copy_layer(self):
        if self.document and self.document.active_layers:
            self._copied_layers = [lyr.copy() for lyr in self.document.active_layers]
            self.lbl_status_msg.setText(f"Copied {len(self._copied_layers)} layer(s)")

    def action_paste_layer(self):
        if self.document and self._copied_layers:
            pasted = []
            for lyr in self._copied_layers:
                dup = lyr.copy()
                dup.offset_x += 30
                dup.offset_y += 30
                self.document.add_layer(dup)
                pasted.append(dup)
            self.document.active_layer_ids = [l.id for l in pasted]
            self.document.notify_changed()
            self.lbl_status_msg.setText(f"Pasted {len(pasted)} layer(s)")

    def action_duplicate_layer(self):
        if self.document:
            self.document.duplicate_layers()

    def action_delete_layer(self):
        if self.document and self.document.active_layer_ids:
            self.document.remove_layers(list(self.document.active_layer_ids))

    def action_add_text_layer(self):
        if self.document:
            text_lyr = Layer(name=f"Text {len(self.document.layers)+1}", layer_type="text")
            text_lyr.offset_x = (self.document.canvas_width - 200) / 2.0
            text_lyr.offset_y = (self.document.canvas_height - 100) / 2.0
            self.document.add_layer(text_lyr)

    def action_add_shape_layer(self):
        if self.document:
            shape_lyr = Layer(name=f"Shape {len(self.document.layers)+1}", layer_type="shape")
            shape_lyr.offset_x = (self.document.canvas_width - 200) / 2.0
            shape_lyr.offset_y = (self.document.canvas_height - 200) / 2.0
            self.document.add_layer(shape_lyr)

    # Per-Layer AI Background Removal
    def action_auto_remove(self):
        active = self.document.active_layer
        if active is None or active.image is None:
            QMessageBox.warning(self, "No Layer Selected", "Please select an image layer to run AI Background Removal.")
            return

        self.top_bar.show_progress(10)
        self.top_bar.btn_auto_remove.setEnabled(False)

        model_name = settings.get("ai_model", "RMBG-1.4")
        device = settings.get("ai_device", "Auto")

        self.worker = BackgroundRemovalWorker(active.image, engine_type="AI", model_name=model_name, device=device)
        self.worker.progress.connect(self.top_bar.show_progress)
        self.worker.finished.connect(lambda mask: self._on_ai_finished(mask, active.id))
        self.worker.error.connect(self._on_ai_error)
        self.worker.start()

    def _on_ai_finished(self, mask, layer_id: str):
        self.top_bar.hide_progress()
        self.top_bar.btn_auto_remove.setEnabled(True)
        self.document.update_mask(mask, layer_id=layer_id, description="AI Background Removal")
        self.lbl_status_msg.setText("AI Background Removal complete on selected layer!")

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
        if not self.document.layers:
            QMessageBox.warning(self, "Empty Document", "There are no layers to export.")
            return
        dlg = ExportDialog(self.document, self)
        dlg.exec()

    def action_settings(self):
        dlg = SettingsDialog(self)
        if dlg.exec():
            pass

    def _on_document_changed(self):
        self.act_undo.setEnabled(self.document.undo_stack.can_undo())
        self.act_redo.setEnabled(self.document.undo_stack.can_redo())
        self.canvas.update_canvas()

    def _on_canvas_mouse_moved(self, scene_pos: QPointF, rgb_color: tuple):
        self.lbl_status_color.setText(f"X: {int(scene_pos.x())}, Y: {int(scene_pos.y())}")
        self.lbl_status_zoom.setText(f"Zoom: {int(self.canvas.zoom_factor * 100)}%")

