from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox, QPushButton,
    QSlider, QGroupBox, QSpinBox, QDoubleSpinBox, QCheckBox, QToolButton, QGridLayout
)
from app.core.image_document import ImageDocument
from app.processing.align_utils import AlignUtils

class TransformPanel(QWidget):
    """
    Comprehensive panel for Layer Transforms, Alignment & Distribution, and Canvas Settings.
    """
    apply_crop_signal = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.doc: ImageDocument = None
        self._updating_ui = False
        self._init_ui()

    def set_document(self, doc: ImageDocument):
        self.doc = doc
        if self.doc:
            self.doc.add_change_listener(self.update_panel)
        self.update_panel()

    def _init_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(12)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        # 1. Layer Position & Size Section
        grp_pos = QGroupBox("Layer Transform", self)
        grid_pos = QGridLayout(grp_pos)
        grid_pos.setSpacing(6)

        grid_pos.addWidget(QLabel("X:", self), 0, 0)
        self.spn_pos_x = QDoubleSpinBox(self)
        self.spn_pos_x.setRange(-10000, 10000)
        self.spn_pos_x.valueChanged.connect(self._on_pos_x_changed)
        grid_pos.addWidget(self.spn_pos_x, 0, 1)

        grid_pos.addWidget(QLabel("Y:", self), 0, 2)
        self.spn_pos_y = QDoubleSpinBox(self)
        self.spn_pos_y.setRange(-10000, 10000)
        self.spn_pos_y.valueChanged.connect(self._on_pos_y_changed)
        grid_pos.addWidget(self.spn_pos_y, 0, 3)

        grid_pos.addWidget(QLabel("Scale X:", self), 1, 0)
        self.spn_scale_x = QDoubleSpinBox(self)
        self.spn_scale_x.setRange(0.01, 100.0)
        self.spn_scale_x.setSingleStep(0.05)
        self.spn_scale_x.setValue(1.0)
        self.spn_scale_x.valueChanged.connect(self._on_scale_x_changed)
        grid_pos.addWidget(self.spn_scale_x, 1, 1)

        grid_pos.addWidget(QLabel("Scale Y:", self), 1, 2)
        self.spn_scale_y = QDoubleSpinBox(self)
        self.spn_scale_y.setRange(0.01, 100.0)
        self.spn_scale_y.setSingleStep(0.05)
        self.spn_scale_y.setValue(1.0)
        self.spn_scale_y.valueChanged.connect(self._on_scale_y_changed)
        grid_pos.addWidget(self.spn_scale_y, 1, 3)

        self.chk_lock_aspect = QCheckBox("Lock Aspect Ratio", self)
        self.chk_lock_aspect.setChecked(True)
        self.chk_lock_aspect.toggled.connect(self._on_lock_aspect_toggled)
        grid_pos.addWidget(self.chk_lock_aspect, 2, 0, 1, 4)

        # Rotation
        grid_pos.addWidget(QLabel("Rotation:", self), 3, 0)
        self.spn_rotation = QDoubleSpinBox(self)
        self.spn_rotation.setRange(-360.0, 360.0)
        self.spn_rotation.setSuffix("°")
        self.spn_rotation.valueChanged.connect(self._on_rotation_changed)
        grid_pos.addWidget(self.spn_rotation, 3, 1, 1, 3)

        # Flips
        hbox_flips = QHBoxLayout()
        self.btn_flip_h = QPushButton("↔ Flip H", self)
        self.btn_flip_h.setCheckable(True)
        self.btn_flip_h.clicked.connect(self._on_flip_h)
        hbox_flips.addWidget(self.btn_flip_h)

        self.btn_flip_v = QPushButton("↕ Flip V", self)
        self.btn_flip_v.setCheckable(True)
        self.btn_flip_v.clicked.connect(self._on_flip_v)
        hbox_flips.addWidget(self.btn_flip_v)
        grid_pos.addLayout(hbox_flips, 4, 0, 1, 4)

        layout.addWidget(grp_pos)

        # 2. Alignment & Distribution Section
        grp_align = QGroupBox("Alignment & Distribute", self)
        vbox_align = QVBoxLayout(grp_align)
        vbox_align.setSpacing(6)

        # Align Target Toggle
        hbox_target = QHBoxLayout()
        hbox_target.addWidget(QLabel("Target:", self))
        self.cmb_align_target = QComboBox(self)
        self.cmb_align_target.addItems(["Canvas", "Selection"])
        hbox_target.addWidget(self.cmb_align_target, stretch=1)
        vbox_align.addLayout(hbox_target)

        # Align Buttons Row 1 (Horizontal)
        hbox_a1 = QHBoxLayout()
        btn_al = QPushButton("Left ⇤", self)
        btn_ac = QPushButton("Center ↔", self)
        btn_ar = QPushButton("Right ⇥", self)
        btn_al.clicked.connect(lambda: self._align("left"))
        btn_ac.clicked.connect(lambda: self._align("center"))
        btn_ar.clicked.connect(lambda: self._align("right"))
        for b in [btn_al, btn_ac, btn_ar]: hbox_a1.addWidget(b)
        vbox_align.addLayout(hbox_a1)

        # Align Buttons Row 2 (Vertical)
        hbox_a2 = QHBoxLayout()
        btn_at = QPushButton("Top ⟰", self)
        btn_am = QPushButton("Middle ↕", self)
        btn_ab = QPushButton("Bottom ⟱", self)
        btn_at.clicked.connect(lambda: self._align("top"))
        btn_am.clicked.connect(lambda: self._align("middle"))
        btn_ab.clicked.connect(lambda: self._align("bottom"))
        for b in [btn_at, btn_am, btn_ab]: hbox_a2.addWidget(b)
        vbox_align.addLayout(hbox_a2)

        # Distribute Buttons
        hbox_dist = QHBoxLayout()
        btn_dh = QPushButton("Distribute H ⫶", self)
        btn_dv = QPushButton("Distribute V ⋯", self)
        btn_dh.clicked.connect(lambda: self._distribute("horizontal"))
        btn_dv.clicked.connect(lambda: self._distribute("vertical"))
        hbox_dist.addWidget(btn_dh)
        hbox_dist.addWidget(btn_dv)
        vbox_align.addLayout(hbox_dist)

        layout.addWidget(grp_align)

        # 3. Canvas Size & Presets Section
        grp_canvas = QGroupBox("Canvas Presets & Settings", self)
        grid_canv = QGridLayout(grp_canvas)

        grid_canv.addWidget(QLabel("Preset:", self), 0, 0)
        self.cmb_preset = QComboBox(self)
        self.cmb_preset.addItems([
            "Custom",
            "YouTube Thumbnail (1920×1080)",
            "YouTube Shorts / Reel (1080×1920)",
            "Instagram Post (1080×1080)",
            "Square HD (2048×2048)",
            "HD Standard (1280×720)"
        ])
        self.cmb_preset.currentIndexChanged.connect(self._on_preset_changed)
        grid_canv.addWidget(self.cmb_preset, 0, 1, 1, 3)

        grid_canv.addWidget(QLabel("Width:", self), 1, 0)
        self.spn_canvas_w = QSpinBox(self)
        self.spn_canvas_w.setRange(10, 16000)
        self.spn_canvas_w.setValue(1920)
        grid_canv.addWidget(self.spn_canvas_w, 1, 1)

        grid_canv.addWidget(QLabel("Height:", self), 1, 2)
        self.spn_canvas_h = QSpinBox(self)
        self.spn_canvas_h.setRange(10, 16000)
        self.spn_canvas_h.setValue(1080)
        grid_canv.addWidget(self.spn_canvas_h, 1, 3)

        btn_resize_canv = QPushButton("📐 Resize Canvas", self)
        btn_resize_canv.clicked.connect(self._apply_canvas_resize)
        grid_canv.addWidget(btn_resize_canv, 2, 0, 1, 4)

        # Grid / Snap Controls
        self.chk_grid = QCheckBox("Show Grid", self)
        self.chk_grid.toggled.connect(self._on_grid_toggled)
        self.chk_snap = QCheckBox("Snap to Grid/Edges", self)
        self.chk_snap.setChecked(True)
        self.chk_snap.toggled.connect(self._on_snap_toggled)
        grid_canv.addWidget(self.chk_grid, 3, 0, 1, 2)
        grid_canv.addWidget(self.chk_snap, 3, 2, 1, 2)

        layout.addWidget(grp_canvas)

    def update_panel(self):
        if not self.doc or self._updating_ui:
            return
        self._updating_ui = True
        try:
            self.spn_canvas_w.setValue(self.doc.canvas_width)
            self.spn_canvas_h.setValue(self.doc.canvas_height)
            self.chk_grid.setChecked(self.doc.show_grid)
            self.chk_snap.setChecked(self.doc.snap_enabled)

            active = self.doc.active_layer
            if active:
                self.spn_pos_x.setValue(active.offset_x)
                self.spn_pos_y.setValue(active.offset_y)
                self.spn_scale_x.setValue(active.scale_x)
                self.spn_scale_y.setValue(active.scale_y)
                self.chk_lock_aspect.setChecked(active.lock_aspect)
                self.spn_rotation.setValue(active.rotation)
                self.btn_flip_h.setChecked(active.flip_h)
                self.btn_flip_v.setChecked(active.flip_v)
        finally:
            self._updating_ui = False

    def _on_pos_x_changed(self, val: float):
        if self.doc and self.doc.active_layer and not self._updating_ui:
            self.doc.active_layer.offset_x = val
            self.doc.notify_changed()

    def _on_pos_y_changed(self, val: float):
        if self.doc and self.doc.active_layer and not self._updating_ui:
            self.doc.active_layer.offset_y = val
            self.doc.notify_changed()

    def _on_scale_x_changed(self, val: float):
        if self.doc and self.doc.active_layer and not self._updating_ui:
            active = self.doc.active_layer
            active.scale_x = val
            if active.lock_aspect:
                active.scale_y = val
                self.spn_scale_y.blockSignals(True)
                self.spn_scale_y.setValue(val)
                self.spn_scale_y.blockSignals(False)
            self.doc.notify_changed()

    def _on_scale_y_changed(self, val: float):
        if self.doc and self.doc.active_layer and not self._updating_ui:
            active = self.doc.active_layer
            active.scale_y = val
            if active.lock_aspect:
                active.scale_x = val
                self.spn_scale_x.blockSignals(True)
                self.spn_scale_x.setValue(val)
                self.spn_scale_x.blockSignals(False)
            self.doc.notify_changed()

    def _on_lock_aspect_toggled(self, checked: bool):
        if self.doc and self.doc.active_layer:
            self.doc.active_layer.lock_aspect = checked

    def _on_rotation_changed(self, val: float):
        if self.doc and self.doc.active_layer and not self._updating_ui:
            self.doc.active_layer.rotation = val
            self.doc.notify_changed()

    def _on_flip_h(self, checked: bool):
        if self.doc and self.doc.active_layer:
            self.doc.active_layer.flip_h = checked
            self.doc.notify_changed()

    def _on_flip_v(self, checked: bool):
        if self.doc and self.doc.active_layer:
            self.doc.active_layer.flip_v = checked
            self.doc.notify_changed()

    def _align(self, mode: str):
        if self.doc:
            AlignUtils.align_layers(self.doc, mode=mode, target=self.cmb_align_target.currentText())

    def _distribute(self, orientation: str):
        if self.doc:
            AlignUtils.distribute_layers(self.doc, orientation=orientation)

    def _on_preset_changed(self, idx: int):
        presets = {
            1: (1920, 1080),
            2: (1080, 1920),
            3: (1080, 1080),
            4: (2048, 2048),
            5: (1280, 720)
        }
        if idx in presets:
            w, h = presets[idx]
            self.spn_canvas_w.setValue(w)
            self.spn_canvas_h.setValue(h)

    def _apply_canvas_resize(self):
        if self.doc:
            self.doc.set_canvas_size(self.spn_canvas_w.value(), self.spn_canvas_h.value())

    def _on_grid_toggled(self, checked: bool):
        if self.doc:
            self.doc.show_grid = checked
            self.doc.notify_changed()

    def _on_snap_toggled(self, checked: bool):
        if self.doc:
            self.doc.snap_enabled = checked

