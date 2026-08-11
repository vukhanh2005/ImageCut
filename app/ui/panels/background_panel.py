from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                             QComboBox, QPushButton, QSlider, QColorDialog, QFileDialog, QGroupBox)
from app.core.image_document import ImageDocument
from app.utils.image_utils import load_image

class BackgroundPanel(QWidget):
    """Right side panel tab for controlling background replacement and blur."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.doc: ImageDocument = None

        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(14)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        # Background Type Selector
        grp_type = QGroupBox("Background Type", self)
        vbox_type = QVBoxLayout(grp_type)

        self.combo_bg_type = QComboBox(self)
        self.combo_bg_type.addItems(["Transparent", "Solid", "Image", "Gradient"])
        self.combo_bg_type.currentTextChanged.connect(self._on_bg_type_changed)
        vbox_type.addWidget(self.combo_bg_type)
        layout.addWidget(grp_type)

        # Solid Color Controls
        self.grp_color = QGroupBox("Solid Color", self)
        vbox_color = QVBoxLayout(self.grp_color)
        self.btn_color_pick = QPushButton("🎨 Pick Color", self)
        self.btn_color_pick.clicked.connect(self._pick_solid_color)
        vbox_color.addWidget(self.btn_color_pick)

        # Preset Quick Colors
        hbox_presets = QHBoxLayout()
        for hex_code, name in [("#FFFFFF", "White"), ("#000000", "Black"), ("#3B82F6", "Blue"), ("#22C55E", "Green")]:
            btn_p = QPushButton(name, self)
            btn_p.setStyleSheet(f"background-color: {hex_code}; color: {'#000000' if hex_code=='#FFFFFF' else '#FFFFFF'}; border-radius: 4px;")
            btn_p.clicked.connect(lambda c=False, h=hex_code: self._set_color_hex(h))
            hbox_presets.addWidget(btn_p)
        vbox_color.addLayout(hbox_presets)
        layout.addWidget(self.grp_color)

        # Background Image Import
        self.grp_image = QGroupBox("Background Image", self)
        vbox_img = QVBoxLayout(self.grp_image)
        self.btn_import_bg = QPushButton("🖼️ Import Image", self)
        self.btn_import_bg.clicked.connect(self._import_bg_image)
        vbox_img.addWidget(self.btn_import_bg)
        layout.addWidget(self.grp_image)

        # Background Blur Slider
        grp_blur = QGroupBox("Background Blur", self)
        vbox_blur = QVBoxLayout(grp_blur)
        hbox_lbl = QHBoxLayout()
        hbox_lbl.addWidget(QLabel("Blur Amount:", self))
        self.lbl_blur_val = QLabel("0 px", self)
        hbox_lbl.addWidget(self.lbl_blur_val)
        vbox_blur.addLayout(hbox_lbl)

        self.slider_blur = QSlider(Qt.Orientation.Horizontal, self)
        self.slider_blur.setRange(0, 50)
        self.slider_blur.setValue(0)
        self.slider_blur.valueChanged.connect(self._on_blur_changed)
        vbox_blur.addWidget(self.slider_blur)
        layout.addWidget(grp_blur)

        self._update_visibility()

    def set_document(self, doc: ImageDocument):
        self.doc = doc
        if self.doc:
            self.combo_bg_type.setCurrentText(self.doc.bg_type)
            self.slider_blur.setValue(self.doc.bg_blur)
            self._update_visibility()

    def _on_bg_type_changed(self, bg_type: str):
        if self.doc:
            self.doc.bg_type = bg_type
            self.doc.notify_changed()
        self._update_visibility()

    def _pick_solid_color(self):
        color = QColorDialog.getColor(QColor(255, 255, 255), self, "Select Background Color")
        if color.isValid() and self.doc:
            self.doc.bg_color = (color.red(), color.green(), color.blue())
            self.doc.notify_changed()

    def _set_color_hex(self, hex_code: str):
        col = QColor(hex_code)
        if self.doc:
            self.doc.bg_color = (col.red(), col.green(), col.blue())
            self.doc.notify_changed()

    def _import_bg_image(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Open Background Image", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp)")
        if file_path and self.doc:
            try:
                bg_arr = load_image(file_path)
                self.doc.bg_image = bg_arr
                self.doc.bg_type = "Image"
                self.combo_bg_type.setCurrentText("Image")
                self.doc.notify_changed()
            except Exception as e:
                print(f"Error loading bg image: {e}")

    def _on_blur_changed(self, val: int):
        self.lbl_blur_val.setText(f"{val} px")
        if self.doc:
            self.doc.bg_blur = val
            self.doc.notify_changed()

    def _update_visibility(self):
        bg_type = self.combo_bg_type.currentText()
        self.grp_color.setVisible(bg_type in ("Solid", "Gradient"))
        self.grp_image.setVisible(bg_type == "Image")
