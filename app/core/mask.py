import numpy as np
import cv2
from scipy.ndimage import gaussian_filter
from app.utils.logger import logger

class MaskProcessor:
    """Provides advanced refinement and post-processing filters for alpha masks."""

    @staticmethod
    def feather(mask: np.ndarray, radius: float) -> np.ndarray:
        """Applies Gaussian feathering to mask edges."""
        if radius <= 0 or mask is None:
            return mask
        blurred = gaussian_filter(mask.astype(np.float32), sigma=radius)
        return np.clip(blurred, 0, 255).astype(np.uint8)

    @staticmethod
    def smooth(mask: np.ndarray, kernel_size: int = 5) -> np.ndarray:
        """Applies bilateral/median smoothing to remove noise along mask edges."""
        if kernel_size < 3:
            return mask
        if kernel_size % 2 == 0:
            kernel_size += 1
        return cv2.medianBlur(mask, kernel_size)

    @staticmethod
    def expand_contract(mask: np.ndarray, value: int) -> np.ndarray:
        """
        Expands (value > 0) or Contracts (value < 0) mask boundaries morphologically.
        """
        if value == 0 or mask is None:
            return mask

        abs_val = abs(value)
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (abs_val * 2 + 1, abs_val * 2 + 1))

        if value > 0:
            return cv2.dilate(mask, kernel, iterations=1)
        else:
            return cv2.erode(mask, kernel, iterations=1)

    @staticmethod
    def adjust_edge_contrast(mask: np.ndarray, contrast: float = 1.0) -> np.ndarray:
        """Adjusts sharpness/contrast of anti-aliased mask edges."""
        if contrast == 1.0 or mask is None:
            return mask
        mask_f = mask.astype(np.float32) / 255.0
        centered = (mask_f - 0.5) * contrast + 0.5
        return (np.clip(centered, 0.0, 1.0) * 255.0).astype(np.uint8)

    @staticmethod
    def decontaminate_colors(rgb_image: np.ndarray, mask: np.ndarray, radius: int = 5) -> np.ndarray:
        """
        Removes background color halos from semi-transparent edge pixels
        by extending solid foreground colors into edge regions.
        """
        if radius <= 0 or mask is None or rgb_image is None:
            return rgb_image

        h, w = mask.shape[:2]
        solid_fg = (mask > 200).astype(np.uint8)
        edge_zone = ((mask > 5) & (mask <= 200)).astype(np.uint8)

        if not np.any(edge_zone):
            return rgb_image

        # Inpaint edge pixels using surrounding solid foreground colors
        cleaned_rgb = cv2.inpaint(rgb_image, 1 - solid_fg, inpaintRadius=radius, flags=cv2.INPAINT_TELEA)

        # Blend inpainted colors back into original edge regions based on opacity
        alpha = (mask.astype(np.float32) / 255.0)[..., np.newaxis]
        output_rgb = (rgb_image.astype(np.float32) * alpha + cleaned_rgb.astype(np.float32) * (1.0 - alpha))
        return np.clip(output_rgb, 0, 255).astype(np.uint8)
