import os
import numpy as np
from PIL import Image
from PySide6.QtCore import QThread, Signal
from app.core.image_document import ImageDocument
from app.processing.compositing import composite_document
from app.utils.logger import logger

class ExportWorker(QThread):
    """
    Asynchronous export worker for rendering and writing high-resolution images to disk.
    Supports PNG (transparent/solid), JPG, WEBP with custom quality and resolution.
    """
    finished = Signal(str)  # Output file path
    error = Signal(str)

    def __init__(self, document: ImageDocument, output_path: str, format_str: str = "PNG", quality: int = 95, width: int = None, height: int = None):
        super().__init__()
        self.doc = document
        self.output_path = output_path
        self.format_str = format_str.upper()
        self.quality = quality
        self.target_width = width
        self.target_height = height

    def run(self):
        try:
            # 1. Render composite image
            comp_array = composite_document(self.doc, preview_mode=False)

            # 2. Resize if target dimensions specified
            if self.target_width and self.target_height:
                pil_img = Image.fromarray(comp_array)
                pil_img = pil_img.resize((self.target_width, self.target_height), Image.Resampling.LANCZOS)
            else:
                pil_img = Image.fromarray(comp_array)

            # 3. Handle JPG conversion (no alpha channel)
            if self.format_str == "JPG" or self.format_str == "JPEG":
                if pil_img.mode == "RGBA":
                    # Replace transparent background with chosen bg color or white
                    bg = Image.new("RGB", pil_img.size, self.doc.bg_color)
                    bg.paste(pil_img, mask=pil_img.split()[3])
                    pil_img = bg
                else:
                    pil_img = pil_img.convert("RGB")
                pil_img.save(self.output_path, "JPEG", quality=self.quality)

            elif self.format_str == "WEBP":
                pil_img.save(self.output_path, "WEBP", quality=self.quality)

            else:
                # Default PNG
                pil_img.save(self.output_path, "PNG", compress_level=6)

            logger.info(f"Successfully exported image to {self.output_path}")
            self.finished.emit(self.output_path)
        except Exception as e:
            logger.error(f"Failed to export image: {e}", exc_info=True)
            self.error.emit(str(e))
