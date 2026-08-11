from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                             QComboBox, QPushButton, QSlider, QGroupBox)
from app.core.image_document import ImageDocument

class TransformPanel(QWidget):
    """Right side panel tab for transformations, rotations, flips, and crop presets."""
    apply_crop_signal = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.doc: ImageDocument = None

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(14)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        # Scale & Rotation
        grp_trans = QGroupBox("Transformation", self)
        vbox_trans = QVBoxLayout(grp_trans)

        # Scale
        hbox_scale = QHBoxLayout()
        hbox_scale.addWidget(QLabel("Scale:", self))
        self.lbl_scale = QLabel("100%", self)
        hbox_scale.addWidget(self.lbl_scale)
        vbox_trans.addLayout(hbox_scale)

        self.slider_scale = QSlider(Qt.Orientation.Horizontal, self)
        self.slider_scale.setRange(20, 300)
        self.slider_scale.setValue(100)
        self.slider_scale.valueChanged.connect(self._on_scale_changed)
        vbox_trans.addWidget(self.slider_scale)

        # Rotation
        hbox_rot = QHBoxLayout()
        hbox_rot.addWidget(QLabel("Rotation:", self))
        self.lbl_rot = QLabel("0°", self)
        hbox_rot.addWidget(self.lbl_rot)
        vbox_trans.addLayout(hbox_rot)

        self.slider_rot = QSlider(Qt.Orientation.Horizontal, self)
        self.slider_rot.setRange(-180, 180)
        self.slider_rot.setValue(0)
        self.slider_rot.valueChanged.connect(self._on_rot_changed)
        vbox_trans.addWidget(self.slider_rot)

        # Flips
        hbox_flips = QHBoxLayout()
        self.btn_flip_h = QPushButton("↔️ Flip Horizontal", self)
        self.btn_flip_h.setCheckable(True)
        self.btn_flip_h.clicked.connect(self._on_flip_h)
        hbox_flips.addWidget(self.btn_flip_h)

        self.btn_flip_v = QPushButton("↕️ Flip Vertical", self)
        self.btn_flip_v.setCheckable(True)
        self.btn_flip_v.clicked.connect(self._on_flip_v)
        hbox_flips.addWidget(self.btn_flip_v)
        vbox_trans.addLayout(hbox_flips)

        layout.addWidget(grp_trans)

        # Crop Presets
        grp_crop = QGroupBox("Crop Presets", self)
        vbox_crop = QVBoxLayout(grp_crop)

        self.combo_crop_ratio = QComboBox(self)
        self.combo_crop_ratio.addItems(["Free", "1:1 Square", "4:3 Standard", "3:4 Portrait", "16:9 Widescreen", "9:16 Story"])
        vbox_crop.addWidget(self.combo_crop_ratio)

        self.btn_apply_crop = QPushButton("✂️ Apply Crop Selection", self)
        self.btn_apply_crop.setObjectName("btn_primary")
        self.btn_apply_crop.clicked.connect(self.apply_crop_signal.emit)
        vbox_crop.addWidget(self.btn_apply_crop)

        layout.addWidget(grp_crop)

    def set_document(self, doc: ImageDocument):
        self.doc = doc

    def _on_scale_changed(self, val: int):
        self.lbl_scale.setText(f"{val}%")
        if self.doc:
            self.doc.fg_scale = val / 100.0
            self.doc.notify_changed()

    def _on_rot_changed(self, val: int):
        self.lbl_rot.setText(f"{val}°")
        if self.doc:
            self.doc.fg_rotation = float(val)
            self.doc.notify_changed()

    def _on_flip_h(self, checked: bool):
        if self.doc:
            self.doc.fg_flip_h = checked
            self.doc.notify_changed()

    def _on_flip_v(self, checked: bool):
        if self.doc:
            self.doc.fg_flip_v = checked
            self.doc.notify_changed()
