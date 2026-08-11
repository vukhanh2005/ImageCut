import os
import numpy as np
from PIL import Image
from PySide6.QtGui import QImage, QPixmap
import cv2
from app.utils.logger import logger

def numpy_to_qimage(array: np.ndarray) -> QImage:
    """
    Converts a NumPy ndarray (RGB, RGBA, or Grayscale) to PySide6 QImage.
    Ensures memory safety by copying the array buffer.
    """
    if array is None or array.size == 0:
        return QImage()

    if array.dtype != np.uint8:
        array = np.clip(array, 0, 255).astype(np.uint8)

    height, width = array.shape[:2]

    if array.ndim == 2:
        # Grayscale
        bytes_per_line = width
        qimg = QImage(array.data, width, height, bytes_per_line, QImage.Format.Format_Grayscale8)
        return qimg.copy()
    elif array.ndim == 3:
        channels = array.shape[2]
        if channels == 3:
            # RGB
            bytes_per_line = 3 * width
            qimg = QImage(array.data, width, height, bytes_per_line, QImage.Format.Format_RGB888)
            return qimg.copy()
        elif channels == 4:
            # RGBA
            bytes_per_line = 4 * width
            qimg = QImage(array.data, width, height, bytes_per_line, QImage.Format.Format_RGBA8888)
            return qimg.copy()

    raise ValueError(f"Unsupported array shape for QImage conversion: {array.shape}")

def numpy_to_qpixmap(array: np.ndarray) -> QPixmap:
    """Converts a NumPy ndarray directly to PySide6 QPixmap."""
    qimg = numpy_to_qimage(array)
    return QPixmap.fromImage(qimg)

def qimage_to_numpy(qimage: QImage) -> np.ndarray:
    """Converts a PySide6 QImage to a NumPy ndarray (RGB or RGBA)."""
    if qimage.isNull():
        return np.array([], dtype=np.uint8)

    qimage = qimage.convertToFormat(QImage.Format.Format_RGBA8888)
    width = qimage.width()
    height = qimage.height()

    ptr = qimage.bits()
    array = np.frombuffer(ptr, np.uint8).reshape((height, width, 4))
    return array.copy()

def pil_to_numpy(pil_img: Image.Image) -> np.ndarray:
    """Converts a PIL Image to a NumPy ndarray."""
    return np.array(pil_img)

def numpy_to_pil(array: np.ndarray) -> Image.Image:
    """Converts a NumPy ndarray to a PIL Image."""
    if array.ndim == 2:
        return Image.fromarray(array, mode='L')
    elif array.shape[2] == 3:
        return Image.fromarray(array, mode='RGB')
    elif array.shape[2] == 4:
        return Image.fromarray(array, mode='RGBA')
    raise ValueError(f"Unsupported array shape: {array.shape}")

def load_image(filepath: str) -> np.ndarray:
    """
    Loads an image from file using PIL/OpenCV and returns an RGB or RGBA uint8 NumPy array.
    Supports PNG, JPG, JPEG, WEBP, BMP, TIFF.
    """
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"Image file does not exist: {filepath}")

    try:
        pil_img = Image.open(filepath)
        pil_img = pil_img.convert("RGBA") if pil_img.mode == "RGBA" else pil_img.convert("RGB")
        return np.array(pil_img, dtype=np.uint8)
    except Exception as e:
        logger.error(f"Error loading image {filepath}: {e}", exc_info=True)
        # OpenCV fallback
        img = cv2.imread(filepath, cv2.IMREAD_UNCHANGED)
        if img is None:
            raise ValueError(f"Could not decode image file: {filepath}")
        if img.ndim == 2:
            return cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)
        elif img.shape[2] == 3:
            return cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        elif img.shape[2] == 4:
            return cv2.cvtColor(img, cv2.COLOR_BGRA2RGBA)
        return img

def create_checkerboard_pattern(size: int = 16, square_size: int = 8, c1: int = 240, c2: int = 200) -> np.ndarray:
    """
    Generates an RGB checkerboard pattern array of shape (size, size, 3).
    """
    pattern = np.zeros((size, size, 3), dtype=np.uint8)
    for y in range(size):
        for x in range(size):
            if ((y // square_size) + (x // square_size)) % 2 == 0:
                pattern[y, x] = [c1, c1, c1]
            else:
                pattern[y, x] = [c2, c2, c2]
    return pattern
