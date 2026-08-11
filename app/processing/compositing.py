import numpy as np
import cv2
from typing import Optional, Tuple
from app.core.image_document import ImageDocument
from app.core.layer import Layer
from app.core.mask import MaskProcessor
from app.processing.color_adjust import apply_image_adjustments
from app.utils.image_utils import create_checkerboard_pattern
from app.utils.logger import logger

def composite_document(doc: ImageDocument, preview_mode: bool = True, fast_drag: bool = False) -> np.ndarray:
    """
    Renders a composite RGBA image from an ImageDocument using ROI patch slicing
    and texture caching for high performance (60+ FPS).
    """
    canvas_w = max(1, doc.canvas_width)
    canvas_h = max(1, doc.canvas_height)

    # 1. Initialize Base Canvas Background Buffer (RGBA float32 for high accuracy blending)
    canvas_bg = generate_canvas_background(doc, canvas_h, canvas_w)
    
    # Check for active layer mask preview modes (B&W, Alpha, Overlay)
    active_lyr = doc.active_layer
    if active_lyr and doc.mask_view_mode in ["BlackWhite", "Alpha", "Overlay"]:
        if active_lyr.image is not None:
            h, w = active_lyr.image.shape[:2]
            mask = active_lyr.mask.copy() if active_lyr.mask is not None else np.full((h, w), 255, dtype=np.uint8)
            if doc.mask_view_mode in ["BlackWhite", "Alpha"]:
                bw_mask = cv2.cvtColor(mask, cv2.COLOR_GRAY2RGBA)
                return bw_mask
            elif doc.mask_view_mode == "Overlay":
                fg_rgb = apply_image_adjustments(active_lyr.image, active_lyr.brightness, active_lyr.contrast, active_lyr.saturation)
                inv_mask = (255 - mask).astype(np.float32) / 255.0
                overlay_rgb = fg_rgb.copy()
                overlay_rgb[:, :, 0] = np.clip(overlay_rgb[:, :, 0] + inv_mask * 180, 0, 255).astype(np.uint8)
                return np.dstack((overlay_rgb, np.full((h, w, 1), 255, dtype=np.uint8)))

    canvas_acc = canvas_bg.astype(np.float32)

    # 2. Iterate and composite layers from bottom (index 0) to top
    for lyr in doc.layers:
        if not lyr.visible or lyr.opacity <= 0.0:
            continue

        layer_rgba = render_single_layer(lyr)
        if layer_rgba is None or layer_rgba.size == 0:
            continue

        # Transform layer RGBA buffer to ROI patch coordinates on canvas
        res = transform_layer_to_canvas_roi(layer_rgba, lyr, canvas_w, canvas_h, fast_drag=fast_drag)
        if res is None:
            continue

        patch_rgba, bbox = res

        # Blend ROI patch onto accumulated canvas buffer
        blend_layer_onto_canvas_roi(canvas_acc, patch_rgba, bbox, lyr.opacity, lyr.blend_mode)

    return np.clip(canvas_acc, 0, 255).astype(np.uint8)


def render_single_layer(lyr: Layer) -> Optional[np.ndarray]:
    """Renders a single layer to its local RGBA buffer before canvas transformation (uses texture cache)."""
    if lyr.layer_type == "image":
        if lyr.image is None:
            return None

        # Return cached texture if layer content/mask/adjustments haven't changed
        if not lyr._dirty and lyr._cached_rgba is not None:
            return lyr._cached_rgba

        fg_rgb = lyr.image.copy()
        h, w = fg_rgb.shape[:2]

        # Apply Color Adjustments
        if (lyr.brightness != 0 or lyr.contrast != 0 or lyr.saturation != 0 or
            lyr.exposure != 0 or lyr.temperature != 0 or lyr.sharpness != 0):
            fg_rgb = apply_image_adjustments(
                fg_rgb,
                brightness=lyr.brightness,
                contrast=lyr.contrast,
                saturation=lyr.saturation,
                exposure=lyr.exposure,
                temperature=lyr.temperature,
                sharpness=lyr.sharpness
            )

        # Process & Refine Alpha Mask
        if lyr.mask is not None:
            mask = lyr.mask.copy()
        else:
            mask = np.full((h, w), 255, dtype=np.uint8)

        if lyr.expand_contract_val != 0:
            mask = MaskProcessor.expand_contract(mask, lyr.expand_contract_val)
        if lyr.smooth_kernel > 0:
            mask = MaskProcessor.smooth(mask, lyr.smooth_kernel)
        if lyr.edge_contrast != 1.0:
            mask = MaskProcessor.adjust_edge_contrast(mask, lyr.edge_contrast)
        if lyr.feather_radius > 0:
            mask = MaskProcessor.feather(mask, lyr.feather_radius)
        if lyr.decontaminate:
            fg_rgb = MaskProcessor.decontaminate_colors(fg_rgb, mask)

        rgba_res = np.dstack((fg_rgb, mask))
        lyr._cached_rgba = rgba_res
        lyr._dirty = False
        return rgba_res


    elif lyr.layer_type == "text":
        return render_text_layer(lyr)

    elif lyr.layer_type == "shape":
        return render_shape_layer(lyr)

    return None

def render_text_layer(lyr: Layer) -> np.ndarray:
    """Renders text string to a local RGBA buffer."""
    text = lyr.text_content or "Text"
    font_scale = max(0.5, lyr.font_size / 30.0)
    thickness = 2 if lyr.font_bold else 1
    font = cv2.FONT_HERSHEY_SIMPLEX

    (tw, th), baseline = cv2.getTextSize(text, font, font_scale, thickness)
    w, h = max(10, tw + 20), max(10, th + baseline + 20)

    buf = np.zeros((h, w, 4), dtype=np.uint8)
    r, g, b = lyr.text_color
    cv2.putText(buf, text, (10, h - 10), font, font_scale, (r, g, b, 255), thickness, cv2.LINE_AA)
    return buf

def render_shape_layer(lyr: Layer) -> np.ndarray:
    """Renders basic vector shape to a local RGBA buffer."""
    w, h = 200, 200
    buf = np.zeros((h, w, 4), dtype=np.uint8)
    fill_color = lyr.fill_color
    stroke_color = lyr.stroke_color
    sw = lyr.stroke_width

    if lyr.shape_type == "Rectangle":
        cv2.rectangle(buf, (sw, sw), (w - sw, h - sw), fill_color, -1)
        if sw > 0:
            cv2.rectangle(buf, (0, 0), (w, h), stroke_color, sw)
    elif lyr.shape_type == "Circle":
        center = (w // 2, h // 2)
        radius = min(w, h) // 2 - sw
        cv2.circle(buf, center, radius, fill_color, -1)
        if sw > 0:
            cv2.circle(buf, center, radius + sw // 2, stroke_color, sw)
    return buf

def transform_layer_to_canvas_roi(
    layer_rgba: np.ndarray,
    lyr: Layer,
    canvas_w: int,
    canvas_h: int,
    fast_drag: bool = False
) -> Optional[Tuple[np.ndarray, Tuple[int, int, int, int]]]:
    """
    Warps layer RGBA buffer to canvas coordinates using ROI bounding box slicing.
    Drastically speeds up performance by skipping offscreen/empty pixel warping.
    """
    h, w = layer_rgba.shape[:2]

    # Handle Flip
    img = layer_rgba
    if lyr.flip_h and lyr.flip_v:
        img = cv2.flip(img, -1)
    elif lyr.flip_h:
        img = cv2.flip(img, 1)
    elif lyr.flip_v:
        img = cv2.flip(img, 0)

    # Center of original image
    src_cx, src_cy = w / 2.0, h / 2.0

    # Scaled dimensions
    scaled_w = w * lyr.scale_x
    scaled_h = h * lyr.scale_y

    # Target center on canvas
    dst_cx = lyr.offset_x + scaled_w / 2.0
    dst_cy = lyr.offset_y + scaled_h / 2.0

    # Calculate 4 rotated corners to compute tight ROI bounding box
    half_w, half_h = scaled_w / 2.0, scaled_h / 2.0
    corners = np.array([
        [-half_w, -half_h],
        [ half_w, -half_h],
        [ half_w,  half_h],
        [-half_w,  half_h]
    ], dtype=np.float32)

    rad = np.radians(lyr.rotation)
    cos_a, sin_a = np.cos(rad), np.sin(rad)
    rot_m = np.array([[cos_a, -sin_a], [sin_a, cos_a]], dtype=np.float32)
    rot_corners = corners @ rot_m.T
    rot_corners[:, 0] += dst_cx
    rot_corners[:, 1] += dst_cy

    xmin = max(0, int(np.floor(np.min(rot_corners[:, 0]))) - 1)
    xmax = min(canvas_w, int(np.ceil(np.max(rot_corners[:, 0]))) + 1)
    ymin = max(0, int(np.floor(np.min(rot_corners[:, 1]))) - 1)
    ymax = min(canvas_h, int(np.ceil(np.max(rot_corners[:, 1]))) + 1)

    if xmin >= xmax or ymin >= ymax:
        return None

    box_w = xmax - xmin
    box_h = ymax - ymin

    # Create Affine Matrix relative to ROI top-left (xmin, ymin)
    T1 = np.array([[1, 0, -src_cx], [0, 1, -src_cy], [0, 0, 1]], dtype=np.float32)
    S = np.array([[lyr.scale_x, 0, 0], [0, lyr.scale_y, 0], [0, 0, 1]], dtype=np.float32)
    R = np.array([[cos_a, -sin_a, 0], [sin_a, cos_a, 0], [0, 0, 1]], dtype=np.float32)
    T2_roi = np.array([[1, 0, dst_cx - xmin], [0, 1, dst_cy - ymin], [0, 0, 1]], dtype=np.float32)

    M_3x3_roi = T2_roi @ R @ S @ T1
    M_2x3_roi = M_3x3_roi[:2, :]

    flags = cv2.INTER_NEAREST if fast_drag else cv2.INTER_LINEAR
    patch = cv2.warpAffine(
        img, M_2x3_roi, (box_w, box_h),
        flags=flags,
        borderMode=cv2.BORDER_CONSTANT,
        borderValue=(0, 0, 0, 0)
    )
    return patch, (ymin, ymax, xmin, xmax)

def blend_layer_onto_canvas_roi(
    canvas_acc: np.ndarray,
    layer_rgba: np.ndarray,
    bbox: Tuple[int, int, int, int],
    opacity: float,
    blend_mode: str
):
    """Blends transformed layer patch onto ROI slice of accumulated canvas buffer."""
    ymin, ymax, xmin, xmax = bbox
    roi_acc = canvas_acc[ymin:ymax, xmin:xmax]

    src_rgb = layer_rgba[:, :, :3].astype(np.float32)
    src_a = (layer_rgba[:, :, 3].astype(np.float32) / 255.0) * opacity
    src_a = src_a[:, :, np.newaxis]

    dst_rgb = roi_acc[:, :, :3]
    dst_a = roi_acc[:, :, 3:4] / 255.0

    # Fast Normal Blend Mode path
    if blend_mode == "Normal":
        inv_src_a = 1.0 - src_a
        out_a = src_a + dst_a * inv_src_a
        safe_out_a = np.where(out_a > 0.0001, out_a, 1.0)
        out_rgb = (src_rgb * src_a + dst_rgb * dst_a * inv_src_a) / safe_out_a
        roi_acc[:, :, :3] = np.where(out_a > 0.0001, out_rgb, dst_rgb)
        roi_acc[:, :, 3:4] = out_a * 255.0
        return

    # Advanced Blend Modes
    if blend_mode == "Multiply":
        blended_rgb = (src_rgb * dst_rgb) / 255.0
    elif blend_mode == "Screen":
        blended_rgb = 255.0 - ((255.0 - src_rgb) * (255.0 - dst_rgb)) / 255.0
    elif blend_mode == "Overlay":
        cond = dst_rgb < 128.0
        blended_rgb = np.where(cond, (2.0 * src_rgb * dst_rgb) / 255.0, 255.0 - (2.0 * (255.0 - src_rgb) * (255.0 - dst_rgb)) / 255.0)
    elif blend_mode == "Darken":
        blended_rgb = np.minimum(src_rgb, dst_rgb)
    elif blend_mode == "Lighten":
        blended_rgb = np.maximum(src_rgb, dst_rgb)
    elif blend_mode == "Add":
        blended_rgb = np.minimum(255.0, src_rgb + dst_rgb)
    elif blend_mode == "Difference":
        blended_rgb = np.abs(src_rgb - dst_rgb)
    else:
        blended_rgb = src_rgb

    out_a = src_a + dst_a * (1.0 - src_a)
    safe_out_a = np.where(out_a > 0.0001, out_a, 1.0)

    out_rgb = (blended_rgb * src_a + dst_rgb * dst_a * (1.0 - src_a)) / safe_out_a
    roi_acc[:, :, :3] = np.where(out_a > 0.0001, out_rgb, dst_rgb)
    roi_acc[:, :, 3:4] = out_a * 255.0

def transform_layer_to_canvas(layer_rgba: np.ndarray, lyr: Layer, canvas_w: int, canvas_h: int) -> np.ndarray:
    """Legacy backward compatibility wrapper."""
    res = transform_layer_to_canvas_roi(layer_rgba, lyr, canvas_w, canvas_h)
    if res is None:
        return np.zeros((canvas_h, canvas_w, 4), dtype=np.uint8)
    patch, (ymin, ymax, xmin, xmax) = res
    full = np.zeros((canvas_h, canvas_w, 4), dtype=np.uint8)
    full[ymin:ymax, xmin:xmax] = patch
    return full

def blend_layer_onto_canvas(canvas_acc: np.ndarray, layer_rgba: np.ndarray, opacity: float, blend_mode: str) -> np.ndarray:
    """Legacy backward compatibility wrapper."""
    blend_layer_onto_canvas_roi(canvas_acc, layer_rgba, (0, canvas_acc.shape[0], 0, canvas_acc.shape[1]), opacity, blend_mode)
    return canvas_acc


def generate_canvas_background(doc: ImageDocument, h: int, w: int) -> np.ndarray:
    """Generates initial background RGBA matrix according to document settings."""
    if doc.bg_type == "Solid":
        r, g, b = doc.bg_color
        bg = np.zeros((h, w, 4), dtype=np.uint8)
        bg[:, :, 0] = r
        bg[:, :, 1] = g
        bg[:, :, 2] = b
        bg[:, :, 3] = int(doc.bg_opacity * 255)
        return bg

    elif doc.bg_type == "Gradient":
        r1, g1, b1 = doc.bg_color
        r2, g2, b2 = doc.bg_color_end
        bg = np.zeros((h, w, 4), dtype=np.uint8)
        ys = np.linspace(0, 1, h, dtype=np.float32)[:, np.newaxis]
        bg[:, :, 0] = (r1 * (1.0 - ys) + r2 * ys).astype(np.uint8)
        bg[:, :, 1] = (g1 * (1.0 - ys) + g2 * ys).astype(np.uint8)
        bg[:, :, 2] = (b1 * (1.0 - ys) + b2 * ys).astype(np.uint8)
        bg[:, :, 3] = int(doc.bg_opacity * 255)
        return bg

    elif doc.bg_type == "Image" and doc.bg_image is not None:
        bg_img = doc.bg_image.copy()
        if bg_img.shape[2] == 3:
            alpha_ch = np.full((bg_img.shape[0], bg_img.shape[1], 1), int(doc.bg_opacity * 255), dtype=np.uint8)
            bg_img = np.dstack((bg_img, alpha_ch))
        return cv2.resize(bg_img, (w, h), interpolation=cv2.INTER_LINEAR)

    else:
        # Default / Transparent (Checkerboard display in GUI canvas)
        bg = np.zeros((h, w, 4), dtype=np.uint8)
        return bg

