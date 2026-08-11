from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                             QComboBox, QPushButton, QGroupBox)
from app.utils.settings import settings

class SettingsDialog(QDialog):
    """Dialog window for configuring application preferences."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setFixedSize(420, 360)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(14)

        # AI Configuration
        grp_ai = QGroupBox("AI Background Removal Settings", self)
        vbox_ai = QVBoxLayout(grp_ai)

        hbox_model = QHBoxLayout()
        hbox_model.addWidget(QLabel("Default Model:", self))
        self.combo_model = QComboBox(self)
        self.combo_model.addItems(["RMBG-1.4", "U2Net", "Silueta"])
        self.combo_model.setCurrentText(settings.get("ai_model"))
        hbox_model.addWidget(self.combo_model)
        vbox_ai.addLayout(hbox_model)

        hbox_device = QHBoxLayout()
        hbox_device.addWidget(QLabel("Inference Hardware:", self))
        self.combo_device = QComboBox(self)
        self.combo_device.addItems(["Auto", "CUDA", "CPU"])
        self.combo_device.setCurrentText(settings.get("ai_device"))
        hbox_device.addWidget(self.combo_device)
        vbox_ai.addLayout(hbox_device)

        layout.addWidget(grp_ai)

        # GUI Theme Configuration
        grp_gui = QGroupBox("Interface & Appearance", self)
        vbox_gui = QVBoxLayout(grp_gui)

        hbox_theme = QHBoxLayout()
        hbox_theme.addWidget(QLabel("Theme:", self))
        self.combo_theme = QComboBox(self)
        self.combo_theme.addItems(["Dark", "Light"])
        self.combo_theme.setCurrentText(settings.get("theme"))
        hbox_theme.addWidget(self.combo_theme)
        vbox_gui.addLayout(hbox_theme)

        layout.addWidget(grp_gui)

        # Save / Cancel Buttons
        hbox_btns = QHBoxLayout()
        hbox_btns.addStretch()

        btn_cancel = QPushButton("Cancel", self)
        btn_cancel.clicked.connect(self.reject)
        hbox_btns.addWidget(btn_cancel)

        btn_save = QPushButton("Save Settings", self)
        btn_save.setObjectName("btn_primary")
        btn_save.clicked.connect(self._save_settings)
        hbox_btns.addWidget(btn_save)

        layout.addLayout(hbox_btns)

    def _save_settings(self):
        settings.set("ai_model", self.combo_model.currentText())
        settings.set("ai_device", self.combo_device.currentText())
        settings.set("theme", self.combo_theme.currentText())
        self.accept()
