import numpy as np
import cv2
from app.ai.base import BackgroundRemovalEngine
from app.utils.logger import logger

class ColorKeyEngine(BackgroundRemovalEngine):
    """
    Engine for removing backgrounds based on color similarity (Chroma Key / Lab color space).
    """

    def __init__(self):
        self._key_color = (255, 255, 255)  # RGB
        self._tolerance = 40                 # Color distance threshold
        self._feather = 5                    # Edge softness

    def load(self, model_name: str = "ColorKey", device: str = "Auto") -> bool:
        return True

    def unload(self):
        pass

    def is_loaded(self) -> bool:
        return True

    def process(self, image: np.ndarray, key_color: tuple = None, tolerance: int = 40, feather: int = 5, **kwargs) -> np.ndarray:
        if image is None or image.size == 0:
            raise ValueError("Input image is empty.")

        if key_color is None:
            key_color = self._key_color

        h, w = image.shape[:2]
        if image.shape[2] == 4:
            rgb_img = cv2.cvtColor(image, cv2.COLOR_RGBA2RGB)
        else:
            rgb_img = image.copy()

        # Convert image and key color to CIE LAB color space for perceptual accuracy
        lab_img = cv2.cvtColor(rgb_img, cv2.COLOR_RGB2LAB).astype(np.float32)
        key_rgb_np = np.uint8([[list(key_color)]])
        key_lab = cv2.cvtColor(key_rgb_np, cv2.COLOR_RGB2LAB).astype(np.float32)[0, 0]

        # Calculate Euclidean distance in Lab space
        dist = np.linalg.norm(lab_img - key_lab, axis=2)

        # Distance thresholding
        mask = np.ones((h, w), dtype=np.float32)
        bg_indices = dist <= tolerance
        mask[bg_indices] = 0.0

        # Soft edge feathering near tolerance boundary
        feather_zone = (dist > tolerance) & (dist <= tolerance + feather)
        if np.any(feather_zone) and feather > 0:
            mask[feather_zone] = (dist[feather_zone] - tolerance) / float(feather)

        mask_uint8 = (mask * 255.0).clip(0, 255).astype(np.uint8)
        return mask_uint8
