from typing import Tuple
import numpy as np
import cv2

def crop_image_and_mask(
    image: np.ndarray,
    mask: np.ndarray,
    crop_rect: Tuple[int, int, int, int]
) -> Tuple[np.ndarray, np.ndarray]:
    """Crops image and alpha mask using bounding box tuple (x, y, width, height)."""
    if image is None or crop_rect is None:
        return image, mask

    x, y, w, h = crop_rect
    img_h, img_w = image.shape[:2]

    x1 = max(0, min(x, img_w - 1))
    y1 = max(0, min(y, img_h - 1))
    x2 = max(x1 + 1, min(x1 + w, img_w))
    y2 = max(y1 + 1, min(y1 + h, img_h))

    cropped_img = image[y1:y2, x1:x2].copy()
    cropped_mask = mask[y1:y2, x1:x2].copy() if mask is not None else None

    return cropped_img, cropped_mask

def apply_transforms(
    image: np.ndarray,
    scale: float = 1.0,
    rotation: float = 0.0,
    flip_h: bool = False,
    flip_v: bool = False
) -> np.ndarray:
    """Applies rotation, scaling, and horizontal/vertical flips to image array."""
    if image is None or image.size == 0:
        return image

    result = image.copy()

    if flip_h:
        result = cv2.flip(result, 1)
    if flip_v:
        result = cv2.flip(result, 0)

    if rotation != 0.0:
        h, w = result.shape[:2]
        center = (w // 2, h // 2)
        matrix = cv2.getRotationMatrix2D(center, rotation, scale)
        result = cv2.warpAffine(result, matrix, (w, h), flags=cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT, borderValue=0)
    elif scale != 1.0:
        h, w = result.shape[:2]
        new_w = max(1, int(w * scale))
        new_h = max(1, int(h * scale))
        result = cv2.resize(result, (new_w, new_h), interpolation=cv2.INTER_LINEAR)

    return result
