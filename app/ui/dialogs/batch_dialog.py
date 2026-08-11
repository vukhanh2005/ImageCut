import os
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (QDialog, QVBoxLayout, QHBoxLayout, QLabel,
                             QComboBox, QPushButton, QLineEdit, QFileDialog, QProgressBar, QListWidget, QGroupBox)
from app.workers.batch_worker import BatchWorker
from app.utils.logger import logger

class BatchDialog(QDialog):
    """Dialog window for batch background removal across multiple files/folders."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Batch Background Removal")
        self.setFixedSize(560, 480)

        self.file_paths = []
        self.worker = None

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        # Input File Queue
        grp_in = QGroupBox("Input Images Queue", self)
        vbox_in = QVBoxLayout(grp_in)

        self.list_files = QListWidget(self)
        vbox_in.addWidget(self.list_files)

        hbox_in_btns = QHBoxLayout()
        btn_add_files = QPushButton("➕ Add Files...", self)
        btn_add_files.clicked.connect(self._add_files)
        hbox_in_btns.addWidget(btn_add_files)

        btn_add_dir = QPushButton("📁 Add Folder...", self)
        btn_add_dir.clicked.connect(self._add_folder)
        hbox_in_btns.addWidget(btn_add_dir)

        btn_clear = QPushButton("🗑️ Clear Queue", self)
        btn_clear.clicked.connect(self._clear_queue)
        hbox_in_btns.addWidget(btn_clear)
        vbox_in.addLayout(hbox_in_btns)

        layout.addWidget(grp_in)

        # Output Folder & Options
        grp_out = QGroupBox("Batch Settings", self)
        vbox_out = QVBoxLayout(grp_out)

        hbox_out_dir = QHBoxLayout()
        hbox_out_dir.addWidget(QLabel("Output Dir:", self))
        self.txt_out_dir = QLineEdit(self)
        self.txt_out_dir.setText(os.path.join(os.path.expanduser("~"), "BackgroundRemover_Output"))
        hbox_out_dir.addWidget(self.txt_out_dir)

        btn_browse_out = QPushButton("Browse...", self)
        btn_browse_out.clicked.connect(self._browse_output)
        hbox_out_dir.addWidget(btn_browse_out)
        vbox_out.addLayout(hbox_out_dir)

        hbox_opts = QHBoxLayout()
        hbox_opts.addWidget(QLabel("AI Model:", self))
        self.combo_model = QComboBox(self)
        self.combo_model.addItems(["RMBG-1.4", "U2Net", "Silueta"])
        hbox_opts.addWidget(self.combo_model)

        hbox_opts.addWidget(QLabel("Format:", self))
        self.combo_fmt = QComboBox(self)
        self.combo_fmt.addItems(["PNG", "JPG", "WEBP"])
        hbox_opts.addWidget(self.combo_fmt)
        vbox_out.addLayout(hbox_opts)

        layout.addWidget(grp_out)

        # Progress Section
        self.progress_bar = QProgressBar(self)
        self.progress_bar.setValue(0)
        layout.addWidget(self.progress_bar)

        self.lbl_status = QLabel("Ready to process.", self)
        layout.addWidget(self.lbl_status)

        # Action Buttons
        hbox_actions = QHBoxLayout()
        hbox_actions.addStretch()
        self.btn_start = QPushButton("⚡ Start Batch", self)
        self.btn_start.setObjectName("btn_primary")
        self.btn_start.clicked.connect(self._start_batch)
        hbox_actions.addWidget(self.btn_start)

        btn_close = QPushButton("Close", self)
        btn_close.clicked.connect(self.accept)
        hbox_actions.addWidget(btn_close)
        layout.addLayout(hbox_actions)

    def _add_files(self):
        files, _ = QFileDialog.getOpenFileNames(self, "Select Images", "", "Images (*.png *.jpg *.jpeg *.webp *.bmp *.tiff)")
        for f in files:
            if f not in self.file_paths:
                self.file_paths.append(f)
                self.list_files.addItem(os.path.basename(f))
        self.lbl_status.setText(f"{len(self.file_paths)} files queued.")

    def _add_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Image Directory")
        if folder:
            valid_exts = (".png", ".jpg", ".jpeg", ".webp", ".bmp", ".tiff")
            count = 0
            for root, _, files in os.walk(folder):
                for file in files:
                    if file.lower().endswith(valid_exts):
                        full_p = os.path.join(root, file)
                        if full_p not in self.file_paths:
                            self.file_paths.append(full_p)
                            self.list_files.addItem(file)
                            count += 1
            self.lbl_status.setText(f"Added {count} files. Total queued: {len(self.file_paths)}")

    def _clear_queue(self):
        self.file_paths.clear()
        self.list_files.clear()
        self.lbl_status.setText("Queue cleared.")
        self.progress_bar.setValue(0)

    def _browse_output(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Output Directory")
        if folder:
            self.txt_out_dir.setText(folder)

    def _start_batch(self):
        if not self.file_paths:
            self.lbl_status.setText("Error: Queue is empty.")
            return

        out_dir = self.txt_out_dir.text()
        model_name = self.combo_model.currentText()
        fmt = self.combo_fmt.currentText()

        self.btn_start.setEnabled(False)
        self.worker = BatchWorker(self.file_paths, out_dir, model_name=model_name, output_format=fmt)
        self.worker.progress.connect(self._on_progress)
        self.worker.finished.connect(self._on_finished)
        self.worker.start()

    def _on_progress(self, idx: int, total: int, filename: str):
        pct = int((idx / float(total)) * 100)
        self.progress_bar.setValue(pct)
        self.lbl_status.setText(f"Processing ({idx}/{total}): {filename}")

    def _on_finished(self, success: int, fail: int):
        self.btn_start.setEnabled(True)
        self.lbl_status.setText(f"Batch completed! Success: {success}, Failed: {fail}")
