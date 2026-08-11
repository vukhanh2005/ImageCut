from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                             QComboBox, QSlider, QCheckBox, QGroupBox)
from app.core.image_document import ImageDocument

class MaskPanel(QWidget):
    """Right side panel tab for mask preview modes and edge refinement filters."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.doc: ImageDocument = None

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(14)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        # Mask View Mode
        grp_view = QGroupBox("Mask View Mode", self)
        vbox_view = QVBoxLayout(grp_view)
        self.combo_view_mode = QComboBox(self)
        self.combo_view_mode.addItems(["Normal", "Overlay", "BlackWhite", "Alpha"])
        self.combo_view_mode.currentTextChanged.connect(self._on_view_mode_changed)
        vbox_view.addWidget(self.combo_view_mode)
        layout.addWidget(grp_view)

        # Edge Refinement
        grp_refine = QGroupBox("Edge Refinement & Matting", self)
        vbox_refine = QVBoxLayout(grp_refine)

        # Feather
        vbox_refine.addLayout(self._create_slider_row("Feather:", 0, 30, 0, self._on_feather_changed))
        # Smoothness
        vbox_refine.addLayout(self._create_slider_row("Smoothness:", 0, 15, 0, self._on_smooth_changed))
        # Expand / Contract
        vbox_refine.addLayout(self._create_slider_row("Expand/Contract:", -20, 20, 0, self._on_expand_changed))
        # Edge Contrast
        vbox_refine.addLayout(self._create_slider_row("Edge Contrast:", 5, 20, 10, self._on_contrast_changed))

        # Color Decontamination
        self.chk_decontam = QCheckBox("Color Decontamination (Hair Halo Clean)", self)
        self.chk_decontam.toggled.connect(self._on_decontam_toggled)
        vbox_refine.addWidget(self.chk_decontam)

        layout.addWidget(grp_refine)

    def _create_slider_row(self, label_text: str, min_val: int, max_val: int, init_val: int, callback):
        hbox = QHBoxLayout()
        lbl = QLabel(label_text, self)
        lbl_val = QLabel(str(init_val), self)
        lbl_val.setFixedWidth(30)
        hbox.addWidget(lbl)
        hbox.addWidget(lbl_val)

        slider = QSlider(Qt.Orientation.Horizontal, self)
        slider.setRange(min_val, max_val)
        slider.setValue(init_val)

        def _val_cb(v):
            lbl_val.setText(str(v))
            callback(v)

        slider.valueChanged.connect(_val_cb)
        hbox.addWidget(slider)
        return hbox

    def set_document(self, doc: ImageDocument):
        self.doc = doc
        if self.doc:
            self.combo_view_mode.setCurrentText(self.doc.mask_view_mode)
            self.chk_decontam.setChecked(self.doc.decontaminate)

    def _on_view_mode_changed(self, mode: str):
        if self.doc:
            self.doc.mask_view_mode = mode
            self.doc.notify_changed()

    def _on_feather_changed(self, val: int):
        if self.doc:
            self.doc.feather_radius = float(val)
            self.doc.notify_changed()

    def _on_smooth_changed(self, val: int):
        if self.doc:
            self.doc.smooth_kernel = val
            self.doc.notify_changed()

    def _on_expand_changed(self, val: int):
        if self.doc:
            self.doc.expand_contract_val = val
            self.doc.notify_changed()

    def _on_contrast_changed(self, val: int):
        if self.doc:
            self.doc.edge_contrast = val / 10.0
            self.doc.notify_changed()

    def _on_decontam_toggled(self, checked: bool):
        if self.doc:
            self.doc.decontaminate = checked
            self.doc.notify_changed()
