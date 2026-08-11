import os
import tempfile
import pytest
import numpy as np
from app.core.image_document import ImageDocument
from app.core.layer import Layer
from app.core.project import ProjectManager
from app.processing.compositing import composite_document
from app.processing.align_utils import AlignUtils

def test_multi_layer_import_and_selection():
    # 1. Create Canvas 1920x1080
    doc = ImageDocument(canvas_width=1920, canvas_height=1080)
    assert doc.width() == 1920
    assert doc.height() == 1080

    # 2. Import 5 images -> 5 layers
    img1 = np.full((100, 100, 3), 50, dtype=np.uint8)
    img2 = np.full((150, 150, 3), 100, dtype=np.uint8)
    img3 = np.full((200, 200, 3), 150, dtype=np.uint8)
    img4 = np.full((250, 250, 3), 200, dtype=np.uint8)
    img5 = np.full((300, 300, 3), 250, dtype=np.uint8)

    l1 = doc.add_image_layer(img1, name="Character")
    l2 = doc.add_image_layer(img2, name="Sword")
    l3 = doc.add_image_layer(img3, name="Player")
    l4 = doc.add_image_layer(img4, name="Object")
    l5 = doc.add_image_layer(img5, name="Background")

    # 3. Check 5 independent layers
    assert len(doc.layers) == 5
    assert [l.name for l in doc.layers] == ["Character", "Sword", "Player", "Object", "Background"]

    # 4. Select single layer
    doc.select_layer(l1.id)
    assert doc.active_layer.id == l1.id

    # 5. Move layer
    l1.offset_x = 120.0
    l1.offset_y = 200.0
    assert l1.offset_x == 120.0

    # 6. Resize layer
    l1.scale_x = 2.0
    l1.scale_y = 2.0
    assert l1.scale_x == 2.0

    # 7. Rotate layer
    l1.rotation = 45.0
    assert l1.rotation == 45.0

    # 8. Change opacity
    l1.opacity = 0.75
    assert l1.opacity == 0.75

    # 9. Hide / Show layer
    l1.visible = False
    assert not l1.visible
    l1.visible = True
    assert l1.visible

    # 10. Lock layer
    l1.locked = True
    assert l1.locked

    # 11. Rename layer
    l1.name = "Hero Character"
    assert l1.name == "Hero Character"

    # 12. Duplicate layer
    dups = doc.duplicate_layers([l1.id])
    assert len(dups) == 1
    assert len(doc.layers) == 6

    # 13. Delete layer
    doc.remove_layers([dups[0].id])
    assert len(doc.layers) == 5

    # 14. Z-Order: Move to Bottom & Top
    doc.move_layer_bottom(l5.id)
    assert doc.layers[0].id == l5.id  # Background moved to bottom

    doc.move_layer_top(l5.id)
    assert doc.layers[-1].id == l5.id

    # 15. Move Up & Down
    doc.move_layer_down(l5.id)
    assert doc.get_layer_index(l5.id) == 3

    # 16. Multi-select layers
    doc.select_layer(l1.id)
    doc.select_layer(l2.id, multi_select=True)
    assert len(doc.active_layers) == 2

    # 17. Group & Ungroup
    grp = doc.group_layers([l1.id, l2.id], group_name="Hero Group")
    assert grp is not None
    assert grp.layer_type == "group"
    assert l1.parent_id == grp.id

    doc.ungroup_layer(grp.id)
    assert l1.parent_id is None

    # 18. Alignment & Distribution
    AlignUtils.align_layers(doc, mode="center", target="Canvas")
    AlignUtils.distribute_layers(doc, orientation="horizontal")

    # 19. Add Mask per layer
    mask = np.full((100, 100), 128, dtype=np.uint8)
    doc.update_mask(mask, layer_id=l1.id)
    assert np.array_equal(l1.mask, mask)

    # 20. Composite rendering output test
    comp = composite_document(doc)
    assert comp.shape == (1080, 1920, 4)

def test_multi_layer_project_roundtrip():
    # Test saving & loading full scene with all layer attributes
    doc = ImageDocument(canvas_width=1280, canvas_height=720)

    imgA = np.full((80, 80, 3), 220, dtype=np.uint8)
    imgB = np.full((120, 120, 3), 80, dtype=np.uint8)

    layerA = doc.add_image_layer(imgA, name="Layer A")
    layerB = doc.add_image_layer(imgB, name="Layer B")

    layerA.offset_x = 50.0
    layerA.offset_y = 80.0
    layerA.scale_x = 1.5
    layerA.scale_y = 1.5
    layerA.rotation = 90.0
    layerA.opacity = 0.8
    layerA.blend_mode = "Multiply"

    with tempfile.TemporaryDirectory() as tmp_dir:
        proj_path = os.path.join(tmp_dir, "multilayer_test.bgrem")
        saved = ProjectManager.save_project(doc, proj_path)
        assert saved
        assert os.path.exists(proj_path)

        loaded_doc = ProjectManager.load_project(proj_path)
        assert loaded_doc.canvas_width == 1280
        assert loaded_doc.canvas_height == 720
        assert len(loaded_doc.layers) == 2

        lA = loaded_doc.layers[0]
        assert lA.name == "Layer A"
        assert lA.offset_x == 50.0
        assert lA.offset_y == 80.0
        assert lA.scale_x == 1.5
        assert lA.rotation == 90.0
        assert lA.opacity == 0.8
        assert lA.blend_mode == "Multiply"
