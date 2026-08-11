from PySide6.QtCore import Qt, Signal, QSize
from PySide6.QtGui import QIcon, QPixmap, QColor, QPainter, QImage, QStandardItemModel, QStandardItem
from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QSlider,
    QComboBox, QListWidget, QListWidgetItem, QLineEdit, QToolButton, QAbstractItemView, QMenu
)
import numpy as np
from app.core.image_document import ImageDocument
from app.core.layer import Layer
from app.utils.image_utils import numpy_to_qpixmap
from app.utils.logger import logger

class LayerItemWidget(QWidget):
    """
    Custom widget item representing a Layer row in the Layer Manager list.
    Displays: Eye (Visibility), Lock, Thumbnail, Name, and Active Indicator.
    """
    visibility_changed = Signal(str, bool)
    lock_changed = Signal(str, bool)
    name_changed = Signal(str, str)

    def __init__(self, layer: Layer, parent=None):
        super().__init__(parent)
        self.layer_id = layer.id
        self._init_ui(layer)

    def _init_ui(self, layer: Layer):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(4, 2, 4, 2)
        layout.setSpacing(6)

        # 1. Eye Visibility Toggle Button
        self.btn_eye = QToolButton(self)
        self.btn_eye.setText("👁" if layer.visible else "🙈")
        self.btn_eye.setFixedSize(26, 26)
        self.btn_eye.setStyleSheet("QToolButton { border: none; background: transparent; font-size: 14px; }")
        self.btn_eye.clicked.connect(self._toggle_visibility)
        layout.addWidget(self.btn_eye)

        # 2. Lock Toggle Button
        self.btn_lock = QToolButton(self)
        self.btn_lock.setText("🔒" if layer.locked else "🔓")
        self.btn_lock.setFixedSize(24, 24)
        self.btn_lock.setStyleSheet("QToolButton { border: none; background: transparent; font-size: 12px; }")
        self.btn_lock.clicked.connect(self._toggle_lock)
        layout.addWidget(self.btn_lock)

        # 3. Layer Thumbnail Preview
        self.lbl_thumb = QLabel(self)
        self.lbl_thumb.setFixedSize(32, 32)
        self.lbl_thumb.setStyleSheet("QLabel { background: #2b2b2b; border: 1px solid #444; border-radius: 4px; }")
        self.update_thumbnail(layer)
        layout.addWidget(self.lbl_thumb)

        # 4. Layer Name (Editable on double-click or inline)
        self.lbl_name = QLabel(layer.name, self)
        self.lbl_name.setStyleSheet("QLabel { font-weight: bold; color: #e0e0e0; }")
        layout.addWidget(self.lbl_name, stretch=1)

        # Inline LineEdit for renaming (hidden by default)
        self.txt_name = QLineEdit(layer.name, self)
        self.txt_name.hide()
        self.txt_name.editingFinished.connect(self._finish_rename)
        layout.addWidget(self.txt_name, stretch=1)

    def update_thumbnail(self, layer: Layer):
        """Generates a 32x32 thumbnail preview of the layer image."""
        if layer.image is not None:
            h, w = layer.image.shape[:2]
            if layer.mask is not None:
                rgba = np.dstack((layer.image, layer.mask))
            else:
                alpha = np.full((h, w, 1), 255, dtype=np.uint8)
                rgba = np.dstack((layer.image, alpha))
            pix = numpy_to_qpixmap(rgba).scaled(32, 32, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation)
            self.lbl_thumb.setPixmap(pix)
        else:
            self.lbl_thumb.setText("📁" if layer.layer_type == "group" else "T")
            self.lbl_thumb.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def _toggle_visibility(self):
        visible = (self.btn_eye.text() == "🙈")
        self.btn_eye.setText("👁" if visible else "🙈")
        self.visibility_changed.emit(self.layer_id, visible)

    def _toggle_lock(self):
        locked = (self.btn_lock.text() == "🔓")
        self.btn_lock.setText("🔒" if locked else "🔓")
        self.lock_changed.emit(self.layer_id, locked)

    def start_rename(self):
        self.lbl_name.hide()
        self.txt_name.show()
        self.txt_name.setFocus()
        self.txt_name.selectAll()

    def _finish_rename(self):
        new_name = self.txt_name.text().strip()
        if new_name:
            self.lbl_name.setText(new_name)
            self.name_changed.emit(self.layer_id, new_name)
        self.txt_name.hide()
        self.lbl_name.show()


class LayerManagerPanel(QWidget):
    """
    Sidebar control panel for Layer Management.
    Supports Z-order drag & drop, visibility, locking, opacity, blend modes,
    grouping, duplicating, deleting, and reordering.
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.document: ImageDocument = None
        self._is_updating_ui = False
        self._init_ui()

    def set_document(self, doc: ImageDocument):
        self.document = doc
        if self.document:
            self.document.add_change_listener(self.update_panel)
        self.update_panel()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(6)

        # Panel Header & Layer Actions Toolbar
        hdr_layout = QHBoxLayout()
        lbl_title = QLabel("Layers", self)
        lbl_title.setStyleSheet("font-size: 14px; font-weight: bold;")
        hdr_layout.addWidget(lbl_title, stretch=1)

        self.btn_add = QToolButton(self)
        self.btn_add.setText("➕")
        self.btn_add.setToolTip("Import New Image Layer")
        self.btn_add.setFixedSize(26, 26)

        self.btn_group = QToolButton(self)
        self.btn_group.setText("📁")
        self.btn_group.setToolTip("Group Selected Layers")
        self.btn_group.setFixedSize(26, 26)

        self.btn_dup = QToolButton(self)
        self.btn_dup.setText("📋")
        self.btn_dup.setToolTip("Duplicate Layer (Ctrl+D)")
        self.btn_dup.setFixedSize(26, 26)

        self.btn_del = QToolButton(self)
        self.btn_del.setText("🗑")
        self.btn_del.setToolTip("Delete Layer (Del)")
        self.btn_del.setFixedSize(26, 26)

        hdr_layout.addWidget(self.btn_add)
        hdr_layout.addWidget(self.btn_group)
        hdr_layout.addWidget(self.btn_dup)
        hdr_layout.addWidget(self.btn_del)
        layout.addLayout(hdr_layout)

        # Z-Order Quick Buttons (Top, Up, Down, Bottom)
        z_layout = QHBoxLayout()
        z_layout.setSpacing(4)
        self.btn_top = QPushButton("Top ⏫", self)
        self.btn_up = QPushButton("Up ▲", self)
        self.btn_down = QPushButton("Down ▼", self)
        self.btn_bot = QPushButton("Bottom ⏬", self)

        for btn in [self.btn_top, self.btn_up, self.btn_down, self.btn_bot]:
            btn.setFixedHeight(24)
            btn.setStyleSheet("font-size: 11px; padding: 2px 4px;")
            z_layout.addWidget(btn)
        layout.addLayout(z_layout)

        # Layer Stack List Widget (Drag & Drop Reordering)
        self.layer_list = QListWidget(self)
        self.layer_list.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.layer_list.setDragDropMode(QAbstractItemView.DragDropMode.InternalMove)
        self.layer_list.setStyleSheet("""
            QListWidget { background: #1e1e1e; border: 1px solid #3a3a3a; border-radius: 4px; }
            QListWidget::item { padding: 2px; border-bottom: 1px solid #2a2a2a; }
            QListWidget::item:selected { background: #005fb8; border-radius: 4px; }
        """)
        self.layer_list.itemSelectionChanged.connect(self._on_selection_changed)
        self.layer_list.model().rowsMoved.connect(self._on_rows_moved)
        self.layer_list.itemDoubleClicked.connect(self._on_item_double_clicked)
        self.layer_list.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.layer_list.customContextMenuRequested.connect(self._show_context_menu)
        layout.addWidget(self.layer_list, stretch=1)

        # Opacity & Blend Mode Section
        opts_layout = QVBoxLayout()
        opts_layout.setSpacing(4)

        # Opacity Slider
        opac_hdr = QHBoxLayout()
        lbl_opac = QLabel("Opacity:", self)
        self.lbl_opac_val = QLabel("100%", self)
        opac_hdr.addWidget(lbl_opac)
        opac_hdr.addStretch(1)
        opac_hdr.addWidget(self.lbl_opac_val)
        opts_layout.addLayout(opac_hdr)

        self.slider_opacity = QSlider(Qt.Orientation.Horizontal, self)
        self.slider_opacity.setRange(0, 100)
        self.slider_opacity.setValue(100)
        self.slider_opacity.valueChanged.connect(self._on_opacity_changed)
        opts_layout.addWidget(self.slider_opacity)

        # Blend Mode Dropdown
        bm_hdr = QHBoxLayout()
        lbl_bm = QLabel("Blend Mode:", self)
        self.cmb_blend_mode = QComboBox(self)
        self.cmb_blend_mode.addItems(["Normal", "Multiply", "Screen", "Overlay", "Darken", "Lighten", "Add", "Difference"])
        self.cmb_blend_mode.currentTextChanged.connect(self._on_blend_mode_changed)
        bm_hdr.addWidget(lbl_bm)
        bm_hdr.addWidget(self.cmb_blend_mode, stretch=1)
        opts_layout.addLayout(bm_hdr)

        layout.addLayout(opts_layout)

        # Signal connections for buttons
        self.btn_group.clicked.connect(self.action_group)
        self.btn_dup.clicked.connect(self.action_duplicate)
        self.btn_del.clicked.connect(self.action_delete)
        self.btn_top.clicked.connect(lambda: self.document and self.document.move_layer_top())
        self.btn_up.clicked.connect(lambda: self.document and self.document.move_layer_up())
        self.btn_down.clicked.connect(lambda: self.document and self.document.move_layer_down())
        self.btn_bot.clicked.connect(lambda: self.document and self.document.move_layer_bottom())

    def update_panel(self):
        """Refreshes layer list items and selection state from Document model efficiently."""
        if not self.document or self._is_updating_ui:
            return

        self._is_updating_ui = True
        try:
            doc_layer_ids_reversed = [lyr.id for lyr in reversed(self.document.layers)]
            current_item_ids = [self.layer_list.item(i).data(Qt.ItemDataRole.UserRole) for i in range(self.layer_list.count())]

            # Only rebuild list items if layer structure/order/count changed
            if doc_layer_ids_reversed != current_item_ids:
                self.layer_list.clear()

                # Render items in reverse order (Top layer at top of QListWidget)
                for lyr in reversed(self.document.layers):
                    item = QListWidgetItem(self.layer_list)
                    item.setSizeHint(QSize(0, 36))
                    item.setData(Qt.ItemDataRole.UserRole, lyr.id)

                    item_widget = LayerItemWidget(lyr)
                    item_widget.visibility_changed.connect(self._on_item_visibility_changed)
                    item_widget.lock_changed.connect(self._on_item_lock_changed)
                    item_widget.name_changed.connect(self._on_item_name_changed)

                    self.layer_list.setItemWidget(item, item_widget)

            # Update selection state
            for i in range(self.layer_list.count()):
                item = self.layer_list.item(i)
                lid = item.data(Qt.ItemDataRole.UserRole)
                item.setSelected(lid in self.document.active_layer_ids)

            # Update Opacity & Blend Mode from active layer
            active = self.document.active_layer
            if active:
                self.slider_opacity.blockSignals(True)
                self.slider_opacity.setValue(int(active.opacity * 100))
                self.lbl_opac_val.setText(f"{int(active.opacity * 100)}%")
                self.slider_opacity.blockSignals(False)

                self.cmb_blend_mode.blockSignals(True)
                self.cmb_blend_mode.setCurrentText(active.blend_mode)
                self.cmb_blend_mode.blockSignals(False)
        finally:
            self._is_updating_ui = False


    def _on_selection_changed(self):
        if self._is_updating_ui or not self.document:
            return
        selected_ids = []
        for item in self.layer_list.selectedItems():
            lid = item.data(Qt.ItemDataRole.UserRole)
            if lid:
                selected_ids.append(lid)
        if selected_ids:
            self.document.active_layer_ids = selected_ids
            self.document.notify_changed()

    def _on_rows_moved(self, parent, start, end, destination, row):
        """Translates list widget drag-and-drop move into document Z-order reordering."""
        if self._is_updating_ui or not self.document:
            return
        # QListWidget index 0 = top of list = last index in document.layers
        total = self.layer_list.count()
        new_layers = []
        for i in range(total - 1, -1, -1):
            item = self.layer_list.item(i)
            lid = item.data(Qt.ItemDataRole.UserRole)
            lyr = self.document.get_layer_by_id(lid)
            if lyr:
                new_layers.append(lyr)
        self.document.layers = new_layers
        self.document.notify_changed()

    def _on_item_double_clicked(self, item: QListWidgetItem):
        widget = self.layer_list.itemWidget(item)
        if isinstance(widget, LayerItemWidget):
            widget.start_rename()

    def _on_item_visibility_changed(self, layer_id: str, visible: bool):
        lyr = self.document.get_layer_by_id(layer_id)
        if lyr:
            lyr.visible = visible
            self.document.notify_changed()

    def _on_item_lock_changed(self, layer_id: str, locked: bool):
        lyr = self.document.get_layer_by_id(layer_id)
        if lyr:
            lyr.locked = locked
            self.document.notify_changed()

    def _on_item_name_changed(self, layer_id: str, new_name: str):
        lyr = self.document.get_layer_by_id(layer_id)
        if lyr:
            lyr.name = new_name
            self.document.notify_changed()

    def _on_opacity_changed(self, val: int):
        if self.document and self.document.active_layer:
            self.lbl_opac_val.setText(f"{val}%")
            self.document.active_layer.opacity = val / 100.0
            self.document.notify_changed()

    def _on_blend_mode_changed(self, mode: str):
        if self.document and self.document.active_layer:
            self.document.active_layer.blend_mode = mode
            self.document.notify_changed()

    def action_duplicate(self):
        if self.document:
            self.document.duplicate_layers()

    def action_delete(self):
        if self.document and self.document.active_layer_ids:
            self.document.remove_layers(list(self.document.active_layer_ids))

    def action_group(self):
        if self.document and len(self.document.active_layer_ids) >= 1:
            self.document.group_layers(list(self.document.active_layer_ids))

    def _show_context_menu(self, pos):
        item = self.layer_list.itemAt(pos)
        if not item or not self.document:
            return
        lid = item.data(Qt.ItemDataRole.UserRole)
        lyr = self.document.get_layer_by_id(lid)
        if not lyr:
            return

        menu = QMenu(self)
        act_rename = menu.addAction("✏ Rename")
        act_dup = menu.addAction("📋 Duplicate")
        act_del = menu.addAction("🗑 Delete")
        menu.addSeparator()
        act_top = menu.addAction("⏫ Move to Top")
        act_up = menu.addAction("▲ Move Up")
        act_down = menu.addAction("▼ Move Down")
        act_bot = menu.addAction("⏬ Move to Bottom")
        menu.addSeparator()
        act_group = menu.addAction("📁 Group Selected")
        act_ungroup = menu.addAction("📂 Ungroup")

        selected_action = menu.exec_(self.layer_list.mapToGlobal(pos))
        if selected_action == act_rename:
            widget = self.layer_list.itemWidget(item)
            if isinstance(widget, LayerItemWidget):
                widget.start_rename()
        elif selected_action == act_dup:
            self.document.duplicate_layers([lid])
        elif selected_action == act_del:
            self.document.remove_layers([lid])
        elif selected_action == act_top:
            self.document.move_layer_top(lid)
        elif selected_action == act_up:
            self.document.move_layer_up(lid)
        elif selected_action == act_down:
            self.document.move_layer_down(lid)
        elif selected_action == act_bot:
            self.document.move_layer_bottom(lid)
        elif selected_action == act_group:
            self.action_group()
        elif selected_action == act_ungroup:
            self.document.ungroup_layer(lid)
