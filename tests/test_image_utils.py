import pytest
import numpy as np
from PySide6.QtGui import QImage, QPixmap
from app.utils.image_utils import (numpy_to_qimage, qimage_to_numpy, numpy_to_qpixmap,
                                   pil_to_numpy, numpy_to_pil, create_checkerboard_pattern)

def test_numpy_to_qimage_conversion():
    # RGB array
    arr_rgb = np.zeros((100, 100, 3), dtype=np.uint8)
    arr_rgb[:, :, 0] = 255  # Red
    qimg = numpy_to_qimage(arr_rgb)
    assert not qimg.isNull()
    assert qimg.width() == 100
    assert qimg.height() == 100

    # Convert back
    converted_back = qimage_to_numpy(qimg)
    assert converted_back.shape == (100, 100, 4)
    assert converted_back[0, 0, 0] == 255

def test_pil_conversion():
    from PIL import Image
    pil_img = Image.new("RGB", (50, 50), color=(0, 255, 0))
    np_img = pil_to_numpy(pil_img)
    assert np_img.shape == (50, 50, 3)
    assert np_img[0, 0, 1] == 255

    converted_pil = numpy_to_pil(np_img)
    assert converted_pil.size == (50, 50)

def test_checkerboard_pattern():
    cb = create_checkerboard_pattern(size=32, square_size=8)
    assert cb.shape == (32, 32, 3)
    assert cb.dtype == np.uint8
