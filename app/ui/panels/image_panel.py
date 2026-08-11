from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                             QSlider, QPushButton, QGroupBox)
from app.core.image_document import ImageDocument

class ImagePanel(QWidget):
    """Right side panel tab for non-destructive color and tone adjustments."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.doc: ImageDocument = None
        self.sliders = {}

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(14)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        grp_adj = QGroupBox("Color & Tone Adjustments", self)
        vbox_adj = QVBoxLayout(grp_adj)

        vbox_adj.addLayout(self._create_slider("Brightness:", "brightness", -100, 100, 0))
        vbox_adj.addLayout(self._create_slider("Contrast:", "contrast", -100, 100, 0))
        vbox_adj.addLayout(self._create_slider("Saturation:", "saturation", -100, 100, 0))
        vbox_adj.addLayout(self._create_slider("Exposure:", "exposure", -100, 100, 0))
        vbox_adj.addLayout(self._create_slider("Temperature:", "temperature", -100, 100, 0))
        vbox_adj.addLayout(self._create_slider("Sharpness:", "sharpness", 0, 100, 0))

        btn_reset = QPushButton("🔄 Reset Adjustments", self)
        btn_reset.clicked.connect(self.reset_all)
        vbox_adj.addWidget(btn_reset)

        layout.addWidget(grp_adj)

    def _create_slider(self, label_text: str, key: str, min_val: int, max_val: int, init_val: int):
        hbox = QHBoxLayout()
        lbl = QLabel(label_text, self)
        lbl_val = QLabel(str(init_val), self)
        lbl_val.setFixedWidth(35)
        hbox.addWidget(lbl)
        hbox.addWidget(lbl_val)

        slider = QSlider(Qt.Orientation.Horizontal, self)
        slider.setRange(min_val, max_val)
        slider.setValue(init_val)

        def _val_cb(v):
            lbl_val.setText(str(v))
            if self.doc:
                setattr(self.doc, key, v)
                self.doc.notify_changed()

        slider.valueChanged.connect(_val_cb)
        self.sliders[key] = (slider, lbl_val)
        hbox.addWidget(slider)
        return hbox

    def set_document(self, doc: ImageDocument):
        self.doc = doc
        if self.doc:
            for key, (slider, lbl) in self.sliders.items():
                val = getattr(self.doc, key, 0)
                slider.setValue(val)
                lbl.setText(str(val))

    def reset_all(self):
        for key, (slider, lbl) in self.sliders.items():
            slider.setValue(0)
            lbl.setText("0")
            if self.doc:
                setattr(self.doc, key, 0)
        if self.doc:
            self.doc.notify_changed()
