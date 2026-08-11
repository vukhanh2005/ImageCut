import os
import json
import zipfile
import tempfile
import io
import numpy as np
from PIL import Image
from app.core.image_document import ImageDocument
from app.utils.logger import logger

class ProjectManager:
    """Handles saving and opening .bgrem project files."""

    @staticmethod
    def save_project(document: ImageDocument, filepath: str) -> bool:
        """Saves current document into a .bgrem zip container."""
        if document.original_image is None:
            logger.error("Cannot save empty document project.")
            return False

        if not filepath.endswith(".bgrem"):
            filepath += ".bgrem"

        try:
            with zipfile.ZipFile(filepath, "w", zipfile.ZIP_DEFLATED) as zf:
                # 1. Save original image
                orig_pil = Image.fromarray(document.original_image)
                with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_orig:
                    orig_pil.save(tmp_orig.name)
                    zf.write(tmp_orig.name, "original.png")
                os.remove(tmp_orig.name)

                # 2. Save mask
                if document.mask is not None:
                    mask_pil = Image.fromarray(document.mask)
                    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_mask:
                        mask_pil.save(tmp_mask.name)
                        zf.write(tmp_mask.name, "mask.png")
                    os.remove(tmp_mask.name)

                # 3. Save custom background image if exists
                if document.bg_image is not None:
                    bg_pil = Image.fromarray(document.bg_image)
                    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_bg:
                        bg_pil.save(tmp_bg.name)
                        zf.write(tmp_bg.name, "bg_image.png")
                    os.remove(tmp_bg.name)

                # 4. Save metadata JSON
                meta = {
                    "bg_type": document.bg_type,
                    "bg_color": list(document.bg_color),
                    "bg_color_end": list(document.bg_color_end),
                    "bg_blur": document.bg_blur,
                    "bg_opacity": document.bg_opacity,
                    "brightness": document.brightness,
                    "contrast": document.contrast,
                    "saturation": document.saturation,
                    "exposure": document.exposure,
                    "temperature": document.temperature,
                    "sharpness": document.sharpness,
                    "crop_rect": list(document.crop_rect) if document.crop_rect else None,
                    "fg_offset_x": document.fg_offset_x,
                    "fg_offset_y": document.fg_offset_y,
                    "fg_scale": document.fg_scale,
                    "fg_rotation": document.fg_rotation,
                    "fg_flip_h": document.fg_flip_h,
                    "fg_flip_v": document.fg_flip_v
                }

                zf.writestr("project.json", json.dumps(meta, indent=2))

            logger.info(f"Project saved successfully to {filepath}")
            return True
        except Exception as e:
            logger.error(f"Error saving project {filepath}: {e}", exc_info=True)
            return False

    @staticmethod
    def load_project(filepath: str) -> ImageDocument:
        """Loads document state from a .bgrem project file."""
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"Project file not found: {filepath}")

        try:
            with zipfile.ZipFile(filepath, "r") as zf:
                # Read original
                orig_bytes = zf.read("original.png")
                orig_img = np.array(Image.open(io.BytesIO(orig_bytes)).convert("RGB"))

                doc = ImageDocument(orig_img)

                # Read mask if present
                if "mask.png" in zf.namelist():
                    mask_bytes = zf.read("mask.png")
                    mask_pil = Image.open(io.BytesIO(mask_bytes))
                    doc.mask = np.array(mask_pil.convert("L"))

                # Read bg_image if present
                if "bg_image.png" in zf.namelist():
                    bg_bytes = zf.read("bg_image.png")
                    doc.bg_image = np.array(Image.open(io.BytesIO(bg_bytes)).convert("RGB"))

                # Read json
                if "project.json" in zf.namelist():
                    meta = json.loads(zf.read("project.json").decode("utf-8"))
                    doc.bg_type = meta.get("bg_type", "Transparent")
                    doc.bg_color = tuple(meta.get("bg_color", [255, 255, 255]))
                    doc.bg_color_end = tuple(meta.get("bg_color_end", [0, 0, 0]))
                    doc.bg_blur = meta.get("bg_blur", 0)
                    doc.bg_opacity = meta.get("bg_opacity", 1.0)
                    doc.brightness = meta.get("brightness", 0)
                    doc.contrast = meta.get("contrast", 0)
                    doc.saturation = meta.get("saturation", 0)
                    doc.exposure = meta.get("exposure", 0)
                    doc.temperature = meta.get("temperature", 0)
                    doc.sharpness = meta.get("sharpness", 0)
                    if meta.get("crop_rect"):
                        doc.crop_rect = tuple(meta["crop_rect"])
                    doc.fg_offset_x = meta.get("fg_offset_x", 0)
                    doc.fg_offset_y = meta.get("fg_offset_y", 0)
                    doc.fg_scale = meta.get("fg_scale", 1.0)
                    doc.fg_rotation = meta.get("fg_rotation", 0.0)
                    doc.fg_flip_h = meta.get("fg_flip_h", False)
                    doc.fg_flip_v = meta.get("fg_flip_v", False)

            doc.notify_changed()
            logger.info(f"Project loaded successfully from {filepath}")
            return doc
        except Exception as e:
            logger.error(f"Failed to load project {filepath}: {e}", exc_info=True)
            raise e
