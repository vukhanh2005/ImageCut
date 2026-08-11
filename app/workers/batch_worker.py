import os
from typing import List
import numpy as np
from PIL import Image
from PySide6.QtCore import QThread, Signal
from app.ai.onnx_engine import ONNXModelEngine
from app.utils.image_utils import load_image
from app.utils.logger import logger

class BatchWorker(QThread):
    """
    Worker thread to process a list of images, remove background automatically via AI,
    and save transparent PNGs or JPGs into an output folder.
    """
    progress = Signal(int, int, str)  # (current_index, total_count, current_filename)
    finished = Signal(int, int)       # (success_count, fail_count)
    error = Signal(str)

    def __init__(self, file_paths: List[str], output_dir: str, model_name: str = "RMBG-1.4", output_format: str = "PNG", quality: int = 95):
        super().__init__()
        self.file_paths = file_paths
        self.output_dir = output_dir
        self.model_name = model_name
        self.output_format = output_format.upper()
        self.quality = quality
        self._is_cancelled = False

    def cancel(self):
        self._is_cancelled = True

    def run(self):
        os.makedirs(self.output_dir, exist_ok=True)
        engine = ONNXModelEngine()
        engine.load(model_name=self.model_name)

        success_count = 0
        fail_count = 0
        total = len(self.file_paths)

        for idx, file_path in enumerate(self.file_paths):
            if self._is_cancelled:
                break

            filename = os.path.basename(file_path)
            self.progress.emit(idx + 1, total, filename)

            try:
                # Load image
                rgb_img = load_image(file_path)
                # Infer mask
                mask = engine.process(rgb_img)

                # Format output filename
                base_name = os.path.splitext(filename)[0]
                ext = ".png" if self.output_format == "PNG" else (f".{self.output_format.lower()}")
                out_path = os.path.join(self.output_dir, f"{base_name}_nobg{ext}")

                if self.output_format == "PNG":
                    rgba = np.dstack((rgb_img[:, :, :3], mask))
                    pil_out = Image.fromarray(rgba)
                    pil_out.save(out_path, "PNG")
                elif self.output_format in ("JPG", "JPEG"):
                    # White background for JPG
                    bg = np.full_like(rgb_img[:, :, :3], 255, dtype=np.uint8)
                    alpha = (mask.astype(float) / 255.0)[..., np.newaxis]
                    comp = (rgb_img[:, :, :3] * alpha + bg * (1 - alpha)).astype(np.uint8)
                    pil_out = Image.fromarray(comp)
                    pil_out.save(out_path, "JPEG", quality=self.quality)
                else:
                    rgba = np.dstack((rgb_img[:, :, :3], mask))
                    pil_out = Image.fromarray(rgba)
                    pil_out.save(out_path, self.output_format, quality=self.quality)

                success_count += 1
            except Exception as e:
                logger.error(f"Failed to batch process {file_path}: {e}")
                fail_count += 1

        self.finished.emit(success_count, fail_count)
