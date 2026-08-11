import os
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                             QComboBox, QPushButton, QSlider, QLineEdit, QFileDialog, QSpinBox, QCheckBox, QGroupBox)
from app.core.image_document import ImageDocument
from app.workers.export_worker import ExportWorker
from app.utils.settings import settings
from app.utils.logger import logger

class ExportDialog(QDialog):
    """Dialog window for configuring image export options."""

    def __init__(self, document: ImageDocument, parent=None):
        super().__init__(parent)
        self.doc = document
        self.setWindowTitle("Export Image")
        self.setFixedSize(480, 420)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(14)

        # Output File Location
        grp_file = QGroupBox("Export Destination", self)
        vbox_file = QVBoxLayout(grp_file)
        hbox_path = QHBoxLayout()
        self.txt_path = QLineEdit(self)
        default_dir = settings.get("export_folder") or os.path.expanduser("~")
        default_file = os.path.join(default_dir, "export_nobg.png")
        self.txt_path.setText(default_file)
        hbox_path.addWidget(self.txt_path)

        btn_browse = QPushButton("Browse...", self)
        btn_browse.clicked.connect(self._browse_destination)
        hbox_path.addWidget(btn_browse)
        vbox_file.addLayout(hbox_path)
        layout.addWidget(grp_file)

        # Export Format & Quality
        grp_fmt = QGroupBox("Format & Quality", self)
        vbox_fmt = QVBoxLayout(grp_fmt)

        hbox_fmt = QHBoxLayout()
        hbox_fmt.addWidget(QLabel("Format:", self))
        self.combo_format = QComboBox(self)
        self.combo_format.addItems(["PNG", "JPG", "WEBP"])
        self.combo_format.currentTextChanged.connect(self._on_format_changed)
        hbox_fmt.addWidget(self.combo_format)
        vbox_fmt.addLayout(hbox_fmt)

        # Quality slider (for JPG/WEBP)
        self.hbox_qual = QHBoxLayout()
        self.hbox_qual.addWidget(QLabel("Quality:", self))
        self.lbl_qual_val = QLabel("95%", self)
        self.hbox_qual.addWidget(self.lbl_qual_val)
        self.slider_quality = QSlider(Qt.Orientation.Horizontal, self)
        self.slider_quality.setRange(1, 100)
        self.slider_quality.setValue(95)
        self.slider_quality.valueChanged.connect(lambda v: self.lbl_qual_val.setText(f"{v}%"))
        self.hbox_qual.addWidget(self.slider_quality)
        vbox_fmt.addLayout(self.hbox_qual)
        layout.addWidget(grp_fmt)

        # Dimension Resizing
        grp_dim = QGroupBox("Resolution / Dimensions & Presets", self)
        vbox_dim = QVBoxLayout(grp_dim)

        hbox_preset = QHBoxLayout()
        hbox_preset.addWidget(QLabel("Preset:", self))
        self.combo_preset = QComboBox(self)
        self.combo_preset.addItems([
            "Current Canvas Size",
            "YouTube Thumbnail (1920×1080)",
            "YouTube Shorts / Reels (1080×1920)",
            "Instagram Post (1080×1080)",
            "Instagram Story (1080×1920)",
            "Custom"
        ])
        self.combo_preset.currentIndexChanged.connect(self._on_preset_changed)
        hbox_preset.addWidget(self.combo_preset, stretch=1)
        vbox_dim.addLayout(hbox_preset)

        hbox_spins = QHBoxLayout()
        orig_w = self.doc.width() if self.doc else 1920
        orig_h = self.doc.height() if self.doc else 1080

        hbox_spins.addWidget(QLabel("Width:", self))
        self.spin_w = QSpinBox(self)
        self.spin_w.setRange(1, 10000)
        self.spin_w.setValue(orig_w)
        hbox_spins.addWidget(self.spin_w)

        hbox_spins.addWidget(QLabel("Height:", self))
        self.spin_h = QSpinBox(self)
        self.spin_h.setRange(1, 10000)
        self.spin_h.setValue(orig_h)
        hbox_spins.addWidget(self.spin_h)
        vbox_dim.addLayout(hbox_spins)

        self.chk_aspect = QCheckBox("Lock Aspect Ratio", self)
        self.chk_aspect.setChecked(True)
        vbox_dim.addWidget(self.chk_aspect)
        layout.addWidget(grp_dim)


        # Action Buttons
        hbox_btns = QHBoxLayout()
        hbox_btns.addStretch()
        btn_cancel = QPushButton("Cancel", self)
        btn_cancel.clicked.connect(self.reject)
        hbox_btns.addWidget(btn_cancel)

        self.btn_export = QPushButton("💾 Export Now", self)
        self.btn_export.setObjectName("btn_primary")
        self.btn_export.clicked.connect(self._do_export)
        hbox_btns.addWidget(self.btn_export)
        layout.addLayout(hbox_btns)

        self._on_format_changed("PNG")

    def _on_preset_changed(self, idx: int):
        presets = {
            0: (self.doc.width() if self.doc else 1920, self.doc.height() if self.doc else 1080),
            1: (1920, 1080),
            2: (1080, 1920),
            3: (1080, 1080),
            4: (1080, 1920)
        }
        if idx in presets:
            w, h = presets[idx]
            self.spin_w.setValue(w)
            self.spin_h.setValue(h)

    def _on_format_changed(self, fmt: str):

        is_lossy = fmt in ("JPG", "WEBP")
        self.slider_quality.setEnabled(is_lossy)

        # Update extension
        path = self.txt_path.text()
        base = os.path.splitext(path)[0]
        ext = f".{fmt.lower()}"
        self.txt_path.setText(base + ext)

    def _browse_destination(self):
        fmt = self.combo_format.currentText()
        filter_str = f"{fmt} Images (*.{fmt.lower()})"
        file_path, _ = QFileDialog.getSaveFileName(self, "Save Image", self.txt_path.text(), filter_str)
        if file_path:
            self.txt_path.setText(file_path)

    def _do_export(self):
        path = self.txt_path.text()
        fmt = self.combo_format.currentText()
        qual = self.slider_quality.value()
        w = self.spin_w.value()
        h = self.spin_h.value()

        self.btn_export.setEnabled(False)
        self.worker = ExportWorker(self.doc, path, format_str=fmt, quality=qual, width=w, height=h)
        self.worker.finished.connect(self._on_finished)
        self.worker.error.connect(self._on_error)
        self.worker.start()

    def _on_finished(self, out_path: str):
        self.accept()

    def _on_error(self, err_msg: str):
        self.btn_export.setEnabled(True)
        print(f"Export Error: {err_msg}")
