import tempfile
import os
import pytest
import numpy as np
from app.core.image_document import ImageDocument
from app.core.mask import MaskProcessor
from app.core.project import ProjectManager
from app.processing.compositing import composite_document

def test_document_initialization():
    test_img = np.full((100, 100, 3), 128, dtype=np.uint8)
    doc = ImageDocument(test_img)
    assert doc.width() == 100
    assert doc.height() == 100
    assert doc.mask is not None
    assert doc.mask.shape == (100, 100)
    assert np.all(doc.mask == 255)

def test_mask_refinement_filters():
    mask = np.zeros((100, 100), dtype=np.uint8)
    mask[25:75, 25:75] = 255

    # Feather
    feathered = MaskProcessor.feather(mask, radius=5)
    assert feathered.shape == (100, 100)
    assert feathered[25, 25] < 255  # Edge smoothed

    # Expand
    expanded = MaskProcessor.expand_contract(mask, value=5)
    assert np.sum(expanded > 0) > np.sum(mask > 0)

    # Contract
    contracted = MaskProcessor.expand_contract(mask, value=-5)
    assert np.sum(contracted > 0) < np.sum(mask > 0)

def test_compositing_pipeline():
    test_img = np.full((50, 50, 3), 200, dtype=np.uint8)
    doc = ImageDocument(test_img)
    doc.bg_type = "Solid"
    doc.bg_color = (0, 0, 255)  # Blue

    # Half removed mask
    doc.mask[:, :25] = 0
    doc.mask[:, 25:] = 255

    comp = composite_document(doc, preview_mode=False)
    assert comp.shape == (50, 50, 4)
    # Left side should be background (blue)
    assert comp[10, 5, 2] == 255 # Blue channel
    # Right side should be foreground (200)
    assert comp[10, 35, 0] == 200

def test_project_serialization():
    test_img = np.full((60, 60, 3), 100, dtype=np.uint8)
    doc = ImageDocument(test_img)
    doc.brightness = 15
    doc.bg_type = "Solid"

    with tempfile.TemporaryDirectory() as tmp_dir:
        proj_file = os.path.join(tmp_dir, "test.bgrem")
        saved = ProjectManager.save_project(doc, proj_file)
        assert saved
        assert os.path.exists(proj_file)

        # Reload
        loaded_doc = ProjectManager.load_project(proj_file)
        assert loaded_doc.width() == 60
        assert loaded_doc.brightness == 15
        assert loaded_doc.bg_type == "Solid"
