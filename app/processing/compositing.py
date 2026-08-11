import numpy as np
import cv2
from app.core.image_document import ImageDocument
from app.core.mask import MaskProcessor
from app.processing.color_adjust import apply_image_adjustments
from app.utils.image_utils import create_checkerboard_pattern
from app.utils.logger import logger

def composite_document(doc: ImageDocument, preview_mode: bool = True) -> np.ndarray:
    """
    Renders a composite RGBA or RGB image from an ImageDocument.
    Applies color adjustments, mask post-processing filters, background replacement,
    background blur, and mode-specific overlays (Normal, Mask Overlay, B&W, Alpha).
    """
    if doc.original_image is None:
        return np.zeros((300, 300, 4), dtype=np.uint8)

    fg_rgb = doc.original_image.copy()
    h, w = fg_rgb.shape[:2]

    # 1. Apply Image Adjustments to Foreground (Brightness, Contrast, Saturation, Sharpness, Temp, Exposure)
    fg_rgb = apply_image_adjustments(
        fg_rgb,
        brightness=doc.brightness,
        contrast=doc.contrast,
        saturation=doc.saturation,
        exposure=doc.exposure,
        temperature=doc.temperature,
        sharpness=doc.sharpness
    )

    # 2. Refine Alpha Mask
    mask = doc.mask.copy() if doc.mask is not None else np.full((h, w), 255, dtype=np.uint8)

    if doc.expand_contract_val != 0:
        mask = MaskProcessor.expand_contract(mask, doc.expand_contract_val)
    if doc.smooth_kernel > 0:
        mask = MaskProcessor.smooth(mask, doc.smooth_kernel)
    if doc.edge_contrast != 1.0:
        mask = MaskProcessor.adjust_edge_contrast(mask, doc.edge_contrast)
    if doc.feather_radius > 0:
        mask = MaskProcessor.feather(mask, doc.feather_radius)

    if doc.decontaminate:
        fg_rgb = MaskProcessor.decontaminate_colors(fg_rgb, mask)

    # 3. Handle Mask Display Modes
    if doc.mask_view_mode == "BlackWhite":
        bw_mask = cv2.cvtColor(mask, cv2.COLOR_GRAY2RGBA)
        return bw_mask
    elif doc.mask_view_mode == "Alpha":
        alpha_view = cv2.cvtColor(mask, cv2.COLOR_GRAY2RGBA)
        return alpha_view
    elif doc.mask_view_mode == "Overlay":
        # Red semi-transparent tint on removed regions
        overlay_rgb = fg_rgb.copy()
        inv_mask = (255 - mask).astype(np.float32) / 255.0
        overlay_rgb[:, :, 0] = np.clip(overlay_rgb[:, :, 0] + inv_mask * 180, 0, 255).astype(np.uint8)
        alpha_ch = np.full((h, w, 1), 255, dtype=np.uint8)
        return np.dstack((overlay_rgb, alpha_ch))

    # Normal Compositing Mode
    # 4. Generate Background Layer
    bg_rgb = generate_background(doc, h, w)

    # Apply Background Blur if requested (Background is blurred while Foreground stays crisp sharp)
    if doc.bg_blur > 0:
        ksize = doc.bg_blur * 2 + 1
        bg_rgb = cv2.GaussianBlur(bg_rgb, (ksize, ksize), 0)

    # 5. Alpha Blend Foreground and Background
    alpha = (mask.astype(np.float32) / 255.0)[..., np.newaxis]
    comp_rgb = (fg_rgb.astype(np.float32) * alpha + bg_rgb.astype(np.float32) * (1.0 - alpha))
    comp_rgb = np.clip(comp_rgb, 0, 255).astype(np.uint8)

    # Alpha channel for export/preview transparency
    if doc.bg_type == "Transparent":
        return np.dstack((fg_rgb, mask))
    else:
        alpha_ch = np.full((h, w, 1), 255, dtype=np.uint8)
        return np.dstack((comp_rgb, alpha_ch))

def generate_background(doc: ImageDocument, h: int, w: int) -> np.ndarray:
    """Generates the background RGB matrix according to document settings."""
    if doc.bg_type == "Solid":
        r, g, b = doc.bg_color
        bg = np.zeros((h, w, 3), dtype=np.uint8)
        bg[:, :] = [r, g, b]
        return bg

    elif doc.bg_type == "Gradient":
        r1, g1, b1 = doc.bg_color
        r2, g2, b2 = doc.bg_color_end
        bg = np.zeros((h, w, 3), dtype=np.uint8)
        for y in range(h):
            t = y / float(h - 1) if h > 1 else 0.0
            r = int(r1 * (1.0 - t) + r2 * t)
            g = int(g1 * (1.0 - t) + g2 * t)
            b = int(b1 * (1.0 - t) + b2 * t)
            bg[y, :] = [r, g, b]
        return bg

    elif doc.bg_type == "Image" and doc.bg_image is not None:
        bg_img = doc.bg_image.copy()
        if bg_img.shape[2] == 4:
            bg_img = cv2.cvtColor(bg_img, cv2.COLOR_RGBA2RGB)
        # Resize to fit target document dimensions
        return cv2.resize(bg_img, (w, h), interpolation=cv2.INTER_LINEAR)

    else:
        # Default / Transparent (Checkerboard display in GUI)
        return create_checkerboard_pattern(size=max(h, w), square_size=16)[:h, :w]
