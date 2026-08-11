import os
import json
import zipfile
import tempfile
import io
import numpy as np
from PIL import Image
from app.core.image_document import ImageDocument
from app.core.layer import Layer
from app.utils.logger import logger

class ProjectManager:
    """Handles saving and opening multi-layer .bgrem project files."""

    @staticmethod
    def save_project(document: ImageDocument, filepath: str) -> bool:
        """Saves current document layers and settings into a .bgrem zip container."""
        if not document.layers and document.original_image is None:
            logger.error("Cannot save empty document project.")
            return False

        if not filepath.endswith(".bgrem"):
            filepath += ".bgrem"

        try:
            with zipfile.ZipFile(filepath, "w", zipfile.ZIP_DEFLATED) as zf:
                layer_metas = []

                for idx, lyr in enumerate(document.layers):
                    img_filename = f"layer_{idx}_img.png"
                    mask_filename = f"layer_{idx}_mask.png"

                    # 1. Save Layer Image
                    if lyr.image is not None:
                        img_pil = Image.fromarray(lyr.image)
                        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_img:
                            img_pil.save(tmp_img.name)
                            zf.write(tmp_img.name, img_filename)
                        os.remove(tmp_img.name)

                    # 2. Save Layer Mask
                    if lyr.mask is not None:
                        mask_pil = Image.fromarray(lyr.mask)
                        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_mask:
                            mask_pil.save(tmp_mask.name)
                            zf.write(tmp_mask.name, mask_filename)
                        os.remove(tmp_mask.name)

                    # 3. Layer Metadata
                    layer_meta = {
                        "id": lyr.id,
                        "name": lyr.name,
                        "layer_type": lyr.layer_type,
                        "opacity": lyr.opacity,
                        "visible": lyr.visible,
                        "locked": lyr.locked,
                        "blend_mode": lyr.blend_mode,
                        "offset_x": lyr.offset_x,
                        "offset_y": lyr.offset_y,
                        "scale_x": lyr.scale_x,
                        "scale_y": lyr.scale_y,
                        "lock_aspect": lyr.lock_aspect,
                        "rotation": lyr.rotation,
                        "flip_h": lyr.flip_h,
                        "flip_v": lyr.flip_v,
                        "brightness": lyr.brightness,
                        "contrast": lyr.contrast,
                        "saturation": lyr.saturation,
                        "exposure": lyr.exposure,
                        "temperature": lyr.temperature,
                        "sharpness": lyr.sharpness,
                        "feather_radius": lyr.feather_radius,
                        "smooth_kernel": lyr.smooth_kernel,
                        "expand_contract_val": lyr.expand_contract_val,
                        "edge_contrast": lyr.edge_contrast,
                        "decontaminate": lyr.decontaminate,
                        "parent_id": lyr.parent_id,
                        "children_ids": lyr.children_ids,
                        "text_content": lyr.text_content,
                        "font_family": lyr.font_family,
                        "font_size": lyr.font_size,
                        "font_bold": lyr.font_bold,
                        "font_italic": lyr.font_italic,
                        "text_color": list(lyr.text_color),
                        "shape_type": lyr.shape_type,
                        "fill_color": list(lyr.fill_color),
                        "stroke_color": list(lyr.stroke_color),
                        "stroke_width": lyr.stroke_width,
                        "has_img": lyr.image is not None,
                        "has_mask": lyr.mask is not None,
                        "img_file": img_filename,
                        "mask_file": mask_filename
                    }
                    layer_metas.append(layer_meta)

                # Save custom background image if exists
                if document.bg_image is not None:
                    bg_pil = Image.fromarray(document.bg_image)
                    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp_bg:
                        bg_pil.save(tmp_bg.name)
                        zf.write(tmp_bg.name, "bg_image.png")
                    os.remove(tmp_bg.name)

                # Global Metadata JSON
                meta = {
                    "version": 2,
                    "canvas_width": document.canvas_width,
                    "canvas_height": document.canvas_height,
                    "bg_type": document.bg_type,
                    "bg_color": list(document.bg_color),
                    "bg_color_end": list(document.bg_color_end),
                    "bg_blur": document.bg_blur,
                    "bg_opacity": document.bg_opacity,
                    "show_grid": document.show_grid,
                    "grid_size": document.grid_size,
                    "grid_opacity": document.grid_opacity,
                    "show_rulers": document.show_rulers,
                    "show_guides": document.show_guides,
                    "snap_enabled": document.snap_enabled,
                    "active_layer_ids": document.active_layer_ids,
                    "layers": layer_metas
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
                # Check for project.json
                meta = {}
                if "project.json" in zf.namelist():
                    meta = json.loads(zf.read("project.json").decode("utf-8"))

                version = meta.get("version", 1)

                # Legacy v1 single-image project upgrade
                if version == 1 or "layers" not in meta:
                    orig_bytes = zf.read("original.png")
                    orig_img = np.array(Image.open(io.BytesIO(orig_bytes)).convert("RGB"))
                    doc = ImageDocument(orig_img)

                    if "mask.png" in zf.namelist():
                        mask_bytes = zf.read("mask.png")
                        doc.mask = np.array(Image.open(io.BytesIO(mask_bytes)).convert("L"))

                    if "bg_image.png" in zf.namelist():
                        bg_bytes = zf.read("bg_image.png")
                        doc.bg_image = np.array(Image.open(io.BytesIO(bg_bytes)).convert("RGB"))

                    doc.bg_type = meta.get("bg_type", "Transparent")
                    doc.bg_color = tuple(meta.get("bg_color", [255, 255, 255]))
                    doc.bg_color_end = tuple(meta.get("bg_color_end", [0, 0, 0]))
                    doc.bg_blur = meta.get("bg_blur", 0)
                    doc.bg_opacity = meta.get("bg_opacity", 1.0)
                    if doc.active_layer:
                        doc.active_layer.brightness = meta.get("brightness", 0)
                        doc.active_layer.contrast = meta.get("contrast", 0)
                        doc.active_layer.saturation = meta.get("saturation", 0)
                        doc.active_layer.exposure = meta.get("exposure", 0)
                        doc.active_layer.temperature = meta.get("temperature", 0)
                        doc.active_layer.sharpness = meta.get("sharpness", 0)
                        doc.active_layer.offset_x = float(meta.get("fg_offset_x", 0))
                        doc.active_layer.offset_y = float(meta.get("fg_offset_y", 0))
                        doc.active_layer.scale_x = float(meta.get("fg_scale", 1.0))
                        doc.active_layer.scale_y = float(meta.get("fg_scale", 1.0))
                        doc.active_layer.rotation = float(meta.get("fg_rotation", 0.0))
                        doc.active_layer.flip_h = meta.get("fg_flip_h", False)
                        doc.active_layer.flip_v = meta.get("fg_flip_v", False)

                    doc.notify_changed()
                    return doc

                # Multi-Layer v2 Project loading
                doc = ImageDocument(canvas_width=meta.get("canvas_width", 1920), canvas_height=meta.get("canvas_height", 1080))
                doc.bg_type = meta.get("bg_type", "Transparent")
                doc.bg_color = tuple(meta.get("bg_color", [255, 255, 255]))
                doc.bg_color_end = tuple(meta.get("bg_color_end", [0, 0, 0]))
                doc.bg_blur = meta.get("bg_blur", 0)
                doc.bg_opacity = meta.get("bg_opacity", 1.0)
                doc.show_grid = meta.get("show_grid", False)
                doc.grid_size = meta.get("grid_size", 20)
                doc.grid_opacity = meta.get("grid_opacity", 0.3)
                doc.show_rulers = meta.get("show_rulers", False)
                doc.show_guides = meta.get("show_guides", True)
                doc.snap_enabled = meta.get("snap_enabled", True)

                if "bg_image.png" in zf.namelist():
                    bg_bytes = zf.read("bg_image.png")
                    doc.bg_image = np.array(Image.open(io.BytesIO(bg_bytes)).convert("RGB"))

                # Reconstruct Layers
                for lmeta in meta.get("layers", []):
                    lyr = Layer(
                        name=lmeta.get("name", "Layer"),
                        layer_type=lmeta.get("layer_type", "image"),
                        opacity=lmeta.get("opacity", 1.0),
                        visible=lmeta.get("visible", True),
                        locked=lmeta.get("locked", False),
                        blend_mode=lmeta.get("blend_mode", "Normal"),
                        layer_id=lmeta.get("id")
                    )

                    if lmeta.get("has_img") and lmeta.get("img_file") in zf.namelist():
                        img_bytes = zf.read(lmeta["img_file"])
                        lyr.image = np.array(Image.open(io.BytesIO(img_bytes)).convert("RGB"))

                    if lmeta.get("has_mask") and lmeta.get("mask_file") in zf.namelist():
                        mask_bytes = zf.read(lmeta["mask_file"])
                        lyr.mask = np.array(Image.open(io.BytesIO(mask_bytes)).convert("L"))

                    lyr.offset_x = float(lmeta.get("offset_x", 0.0))
                    lyr.offset_y = float(lmeta.get("offset_y", 0.0))
                    lyr.scale_x = float(lmeta.get("scale_x", 1.0))
                    lyr.scale_y = float(lmeta.get("scale_y", 1.0))
                    lyr.lock_aspect = lmeta.get("lock_aspect", True)
                    lyr.rotation = float(lmeta.get("rotation", 0.0))
                    lyr.flip_h = lmeta.get("flip_h", False)
                    lyr.flip_v = lmeta.get("flip_v", False)

                    lyr.brightness = lmeta.get("brightness", 0)
                    lyr.contrast = lmeta.get("contrast", 0)
                    lyr.saturation = lmeta.get("saturation", 0)
                    lyr.exposure = lmeta.get("exposure", 0)
                    lyr.temperature = lmeta.get("temperature", 0)
                    lyr.sharpness = lmeta.get("sharpness", 0)

                    lyr.feather_radius = float(lmeta.get("feather_radius", 0.0))
                    lyr.smooth_kernel = int(lmeta.get("smooth_kernel", 0))
                    lyr.expand_contract_val = int(lmeta.get("expand_contract_val", 0))
                    lyr.edge_contrast = float(lmeta.get("edge_contrast", 1.0))
                    lyr.decontaminate = lmeta.get("decontaminate", False)

                    lyr.parent_id = lmeta.get("parent_id")
                    lyr.children_ids = lmeta.get("children_ids", [])

                    lyr.text_content = lmeta.get("text_content", "Sample Text")
                    lyr.font_family = lmeta.get("font_family", "Arial")
                    lyr.font_size = lmeta.get("font_size", 48)
                    lyr.font_bold = lmeta.get("font_bold", False)
                    lyr.font_italic = lmeta.get("font_italic", False)
                    if lmeta.get("text_color"):
                        lyr.text_color = tuple(lmeta["text_color"])

                    lyr.shape_type = lmeta.get("shape_type", "Rectangle")
                    if lmeta.get("fill_color"):
                        lyr.fill_color = tuple(lmeta["fill_color"])
                    if lmeta.get("stroke_color"):
                        lyr.stroke_color = tuple(lmeta["stroke_color"])
                    lyr.stroke_width = lmeta.get("stroke_width", 2)

                    doc.layers.append(lyr)

                doc.active_layer_ids = meta.get("active_layer_ids", [])
                if not doc.active_layer_ids and doc.layers:
                    doc.active_layer_ids = [doc.layers[-1].id]

            doc.notify_changed()
            logger.info(f"Project loaded successfully from {filepath}")
            return doc
        except Exception as e:
            logger.error(f"Failed to load project {filepath}: {e}", exc_info=True)
            raise e

