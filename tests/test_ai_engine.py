import pytest
import numpy as np
from app.ai.color_key_engine import ColorKeyEngine
from app.ai.onnx_engine import ONNXModelEngine

def test_color_key_engine():
    engine = ColorKeyEngine()
    test_img = np.zeros((100, 100, 3), dtype=np.uint8)
    test_img[:, :, :] = [255, 255, 255] # White background
    test_img[30:70, 30:70, :] = [255, 0, 0] # Red foreground object

    mask = engine.process(test_img, key_color=(255, 255, 255), tolerance=30)
    assert mask.shape == (100, 100)
    # White background should be removed (0)
    assert mask[0, 0] == 0
    # Red object should be kept (255)
    assert mask[50, 50] == 255

def test_onnx_engine_fallback():
    engine = ONNXModelEngine()
    test_img = np.zeros((100, 100, 3), dtype=np.uint8)
    test_img[20:80, 20:80] = 255
    mask = engine.process(test_img)
    assert mask.shape == (100, 100)
    assert mask.dtype == np.uint8
