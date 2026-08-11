import numpy as np
import cv2

def apply_image_adjustments(
    image: np.ndarray,
    brightness: int = 0,
    contrast: int = 0,
    saturation: int = 0,
    exposure: int = 0,
    temperature: int = 0,
    sharpness: int = 0
) -> np.ndarray:
    """
    Applies color & tone adjustments to an RGB uint8 image array.
    """
    if image is None or image.size == 0:
        return image

    img_f = image.astype(np.float32)

    # 1. Brightness & Exposure (-100 to 100)
    if brightness != 0 or exposure != 0:
        total_b = brightness * 1.25 + exposure * 1.5
        img_f += total_b

    # 2. Contrast (-100 to 100)
    if contrast != 0:
        factor = (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast))
        img_f = factor * (img_f - 128.0) + 128.0

    img_f = np.clip(img_f, 0, 255)

    # 3. Saturation (-100 to 100)
    if saturation != 0:
        hsv = cv2.cvtColor(img_f.astype(np.uint8), cv2.COLOR_RGB2HSV).astype(np.float32)
        sat_scale = 1.0 + (saturation / 100.0)
        hsv[:, :, 1] = np.clip(hsv[:, :, 1] * sat_scale, 0, 255)
        img_f = cv2.cvtColor(hsv.astype(np.uint8), cv2.COLOR_HSV2RGB).astype(np.float32)

    # 4. Color Temperature (-100 to 100)
    if temperature != 0:
        # Positive -> Warmer (increase Red, decrease Blue)
        # Negative -> Cooler (increase Blue, decrease Red)
        temp_val = temperature * 0.5
        img_f[:, :, 0] = np.clip(img_f[:, :, 0] + temp_val, 0, 255) # Red
        img_f[:, :, 2] = np.clip(img_f[:, :, 2] - temp_val, 0, 255) # Blue

    result = np.clip(img_f, 0, 255).astype(np.uint8)

    # 5. Sharpness (0 to 100)
    if sharpness > 0:
        kernel_amount = sharpness / 100.0 * 1.5
        blurred = cv2.GaussianBlur(result, (0, 0), sigmaX=3)
        sharpened = cv2.addWeighted(result, 1.0 + kernel_amount, blurred, -kernel_amount, 0)
        result = np.clip(sharpened, 0, 255).astype(np.uint8)

    return result
