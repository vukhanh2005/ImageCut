from typing import List, Callable, Tuple, Optional
import numpy as np
import cv2
from app.core.history import UndoStack, MaskEditCommand, DocumentActionCommand
from app.core.layer import Layer
from app.utils.logger import logger

class ImageDocument:
    """
    Central document manager for Multi-Layer Image Compositing.
    Manages canvas dimensions, layer stack (Z-order), selection, background options,
    view settings (grid, rulers, snap, guides), and non-destructive rendering pipeline.
    """

    def __init__(self, original_image: Optional[np.ndarray] = None, canvas_width: int = 1920, canvas_height: int = 1080):
        self.undo_stack = UndoStack()

        # Canvas Dimensions
        self.canvas_width: int = canvas_width
        self.canvas_height: int = canvas_height

        # Layer Stack (Index 0 = Bottommost Layer, Index -1 = Topmost Layer)
        self.layers: List[Layer] = []
        self.active_layer_ids: List[str] = []

        # Background Configuration
        self.bg_type: str = "Transparent"    # "Transparent", "Solid", "Image", "Gradient"
        self.bg_color: Tuple[int, int, int] = (255, 255, 255) # RGB
        self.bg_color_end: Tuple[int, int, int] = (0, 0, 0)   # For gradient
        self.bg_image: Optional[np.ndarray] = None     # Custom background image
        self.bg_blur: int = 0                # 0 to 100 px
        self.bg_opacity: float = 1.0          # 0.0 to 1.0
        self.bg_offset_x: float = 0.0
        self.bg_offset_y: float = 0.0
        self.bg_scale: float = 1.0
        self.bg_rotation: float = 0.0

        # View & Workspace Grid / Snapping Settings
        self.show_grid: bool = False
        self.grid_size: int = 20  # 10, 20, 50, 100 px
        self.grid_opacity: float = 0.3
        self.show_rulers: bool = False
        self.show_guides: bool = True
        self.snap_enabled: bool = True

        # Mask View Modes (for current active layer)
        self.mask_view_mode: str = "Normal"  # "Normal", "Overlay", "BlackWhite", "Alpha"
        self.mask_opacity: float = 0.5

        # Event listeners
        self._change_listeners: List[Callable] = []

        if original_image is not None:
            self.set_original_image(original_image)

    # Backward compatibility properties & helpers
    @property
    def active_layer(self) -> Optional[Layer]:
        """Returns the primary active layer."""
        if self.active_layer_ids:
            target = self.get_layer_by_id(self.active_layer_ids[-1])
            if target:
                return target
        if self.layers:
            self.active_layer_ids = [self.layers[-1].id]
            return self.layers[-1]
        return None

    @active_layer.setter
    def active_layer(self, layer: Optional[Layer]):
        if layer and layer in self.layers:
            self.active_layer_ids = [layer.id]
        elif layer is None:
            self.active_layer_ids = []

    @property
    def active_layers(self) -> List[Layer]:
        """Returns all selected layers."""
        res = []
        for lid in self.active_layer_ids:
            lyr = self.get_layer_by_id(lid)
            if lyr:
                res.append(lyr)
        if not res and self.layers:
            res = [self.layers[-1]]
            self.active_layer_ids = [self.layers[-1].id]
        return res

    @property
    def original_image(self) -> Optional[np.ndarray]:
        """Legacy property: returns image of current active layer."""
        lyr = self.active_layer
        return lyr.image if lyr else None

    @property
    def mask(self) -> Optional[np.ndarray]:
        """Legacy property: returns mask of current active layer."""
        lyr = self.active_layer
        return lyr.mask if lyr else None

    @mask.setter
    def mask(self, new_mask: Optional[np.ndarray]):
        lyr = self.active_layer
        if lyr:
            lyr.mask = new_mask

    # Forwarding adjustment properties to active layer for backwards compatibility
    @property
    def brightness(self) -> int:
        return self.active_layer.brightness if self.active_layer else 0
    @brightness.setter
    def brightness(self, val: int):
        if self.active_layer: self.active_layer.brightness = val

    @property
    def contrast(self) -> int:
        return self.active_layer.contrast if self.active_layer else 0
    @contrast.setter
    def contrast(self, val: int):
        if self.active_layer: self.active_layer.contrast = val

    @property
    def saturation(self) -> int:
        return self.active_layer.saturation if self.active_layer else 0
    @saturation.setter
    def saturation(self, val: int):
        if self.active_layer: self.active_layer.saturation = val

    @property
    def exposure(self) -> int:
        return self.active_layer.exposure if self.active_layer else 0
    @exposure.setter
    def exposure(self, val: int):
        if self.active_layer: self.active_layer.exposure = val

    @property
    def temperature(self) -> int:
        return self.active_layer.temperature if self.active_layer else 0
    @temperature.setter
    def temperature(self, val: int):
        if self.active_layer: self.active_layer.temperature = val

    @property
    def sharpness(self) -> int:
        return self.active_layer.sharpness if self.active_layer else 0
    @sharpness.setter
    def sharpness(self, val: int):
        if self.active_layer: self.active_layer.sharpness = val

    @property
    def fg_offset_x(self) -> float:
        return self.active_layer.offset_x if self.active_layer else 0.0
    @fg_offset_x.setter
    def fg_offset_x(self, val: float):
        if self.active_layer: self.active_layer.offset_x = val

    @property
    def fg_offset_y(self) -> float:
        return self.active_layer.offset_y if self.active_layer else 0.0
    @fg_offset_y.setter
    def fg_offset_y(self, val: float):
        if self.active_layer: self.active_layer.offset_y = val

    @property
    def fg_scale(self) -> float:
        return self.active_layer.scale_x if self.active_layer else 1.0
    @fg_scale.setter
    def fg_scale(self, val: float):
        if self.active_layer:
            self.active_layer.scale_x = val
            self.active_layer.scale_y = val

    @property
    def fg_rotation(self) -> float:
        return self.active_layer.rotation if self.active_layer else 0.0
    @fg_rotation.setter
    def fg_rotation(self, val: float):
        if self.active_layer: self.active_layer.rotation = val

    @property
    def fg_flip_h(self) -> bool:
        return self.active_layer.flip_h if self.active_layer else False
    @fg_flip_h.setter
    def fg_flip_h(self, val: bool):
        if self.active_layer: self.active_layer.flip_h = val

    @property
    def fg_flip_v(self) -> bool:
        return self.active_layer.flip_v if self.active_layer else False
    @fg_flip_v.setter
    def fg_flip_v(self, val: bool):
        if self.active_layer: self.active_layer.flip_v = val

    @property
    def crop_rect(self) -> Optional[Tuple[int, int, int, int]]:
        if self.active_layer and self.active_layer.image is not None:
            return (0, 0, self.active_layer.width(), self.active_layer.height())
        return None
    @crop_rect.setter
    def crop_rect(self, val):
        pass

    @property
    def feather_radius(self) -> float:
        return self.active_layer.feather_radius if self.active_layer else 0.0
    @feather_radius.setter
    def feather_radius(self, val: float):
        if self.active_layer: self.active_layer.feather_radius = val

    @property
    def smooth_kernel(self) -> int:
        return self.active_layer.smooth_kernel if self.active_layer else 0
    @smooth_kernel.setter
    def smooth_kernel(self, val: int):
        if self.active_layer: self.active_layer.smooth_kernel = val

    @property
    def expand_contract_val(self) -> int:
        return self.active_layer.expand_contract_val if self.active_layer else 0
    @expand_contract_val.setter
    def expand_contract_val(self, val: int):
        if self.active_layer: self.active_layer.expand_contract_val = val

    @property
    def edge_contrast(self) -> float:
        return self.active_layer.edge_contrast if self.active_layer else 1.0
    @edge_contrast.setter
    def edge_contrast(self, val: float):
        if self.active_layer: self.active_layer.edge_contrast = val

    @property
    def decontaminate(self) -> bool:
        return self.active_layer.decontaminate if self.active_layer else False
    @decontaminate.setter
    def decontaminate(self, val: bool):
        if self.active_layer: self.active_layer.decontaminate = val


    # Document & Image Setup
    def set_original_image(self, image: np.ndarray, layer_name: str = "Image 1"):
        """Legacy / single-image import entry point. Adds image as new layer and adjusts canvas if empty."""
        if image is None or image.size == 0:
            return

        if image.shape[2] == 4:
            initial_alpha = image[:, :, 3].copy()
            orig_rgb = image[:, :, :3].copy()
        else:
            orig_rgb = image.copy()
            initial_alpha = np.full((image.shape[0], image.shape[1]), 255, dtype=np.uint8)

        # Set canvas dimensions if this is the first image or document was fresh
        if not self.layers:
            self.canvas_width = image.shape[1]
            self.canvas_height = image.shape[0]

        layer = Layer(name=layer_name, image=orig_rgb)
        layer.mask = initial_alpha

        # Center layer on canvas
        layer.offset_x = (self.canvas_width - layer.width()) / 2.0
        layer.offset_y = (self.canvas_height - layer.height()) / 2.0

        self.add_layer(layer)
        self.notify_changed()

    def add_image_layer(self, image: np.ndarray, name: str = None) -> Layer:
        """Adds a new image layer to the document."""
        if image.shape[2] == 4:
            mask = image[:, :, 3].copy()
            rgb = image[:, :, :3].copy()
        else:
            rgb = image.copy()
            mask = np.full((rgb.shape[0], rgb.shape[1]), 255, dtype=np.uint8)

        idx = len(self.layers) + 1
        layer_name = name or f"Image {idx}"
        layer = Layer(name=layer_name, image=rgb)
        layer.mask = mask

        # Center on canvas
        layer.offset_x = max(0.0, (self.canvas_width - layer.width()) / 2.0)
        layer.offset_y = max(0.0, (self.canvas_height - layer.height()) / 2.0)


        self.add_layer(layer)
        return layer


    # Layer Management
    def add_layer(self, layer: Layer, index: Optional[int] = None) -> Layer:
        if index is None or index >= len(self.layers):
            self.layers.append(layer)
        else:
            self.layers.insert(max(0, index), layer)
        self.active_layer_ids = [layer.id]
        self.notify_changed()
        return layer

    def get_layer_by_id(self, layer_id: str) -> Optional[Layer]:
        for lyr in self.layers:
            if lyr.id == layer_id:
                return lyr
        return None

    def get_layer_index(self, layer_id: str) -> int:
        for idx, lyr in enumerate(self.layers):
            if lyr.id == layer_id:
                return idx
        return -1

    def remove_layers(self, layer_ids: List[str]):
        if not layer_ids:
            return
        self.layers = [lyr for lyr in self.layers if lyr.id not in layer_ids]
        self.active_layer_ids = [lid for lid in self.active_layer_ids if lid not in layer_ids]
        if not self.active_layer_ids and self.layers:
            self.active_layer_ids = [self.layers[-1].id]
        self.notify_changed()

    def duplicate_layers(self, layer_ids: Optional[List[str]] = None) -> List[Layer]:
        target_ids = layer_ids or self.active_layer_ids
        new_layers = []
        for lid in target_ids:
            lyr = self.get_layer_by_id(lid)
            if lyr:
                dup = lyr.copy()
                dup.offset_x += 20
                dup.offset_y += 20
                idx = self.get_layer_index(lid)
                self.layers.insert(idx + 1, dup)
                new_layers.append(dup)
        if new_layers:
            self.active_layer_ids = [l.id for l in new_layers]
            self.notify_changed()
        return new_layers

    # Z-Order Operations
    def reorder_layer(self, layer_id: str, new_index: int):
        idx = self.get_layer_index(layer_id)
        if idx == -1 or idx == new_index:
            return
        new_index = max(0, min(new_index, len(self.layers) - 1))
        lyr = self.layers.pop(idx)
        self.layers.insert(new_index, lyr)
        self.notify_changed()

    def move_layer_up(self, layer_id: Optional[str] = None):
        target_id = layer_id or (self.active_layer.id if self.active_layer else None)
        if not target_id:
            return
        idx = self.get_layer_index(target_id)
        if idx != -1 and idx < len(self.layers) - 1:
            self.reorder_layer(target_id, idx + 1)

    def move_layer_down(self, layer_id: Optional[str] = None):
        target_id = layer_id or (self.active_layer.id if self.active_layer else None)
        if not target_id:
            return
        idx = self.get_layer_index(target_id)
        if idx > 0:
            self.reorder_layer(target_id, idx - 1)

    def move_layer_top(self, layer_id: Optional[str] = None):
        target_id = layer_id or (self.active_layer.id if self.active_layer else None)
        if not target_id:
            return
        self.reorder_layer(target_id, len(self.layers) - 1)

    def move_layer_bottom(self, layer_id: Optional[str] = None):
        target_id = layer_id or (self.active_layer.id if self.active_layer else None)
        if not target_id:
            return
        self.reorder_layer(target_id, 0)

    # Layer Selection
    def select_layer(self, layer_id: str, multi_select: bool = False, toggle: bool = False):
        if not self.get_layer_by_id(layer_id):
            return
        if multi_select:
            if toggle and layer_id in self.active_layer_ids:
                if len(self.active_layer_ids) > 1:
                    self.active_layer_ids.remove(layer_id)
            else:
                if layer_id not in self.active_layer_ids:
                    self.active_layer_ids.append(layer_id)
        else:
            self.active_layer_ids = [layer_id]
        self.notify_changed()

    def select_all(self):
        self.active_layer_ids = [lyr.id for lyr in self.layers]
        self.notify_changed()

    def clear_selection(self):
        self.active_layer_ids = []
        self.notify_changed()

    # Grouping
    def group_layers(self, layer_ids: List[str], group_name: str = "Group") -> Optional[Layer]:
        if len(layer_ids) < 1:
            return None
        valid_ids = [lid for lid in layer_ids if self.get_layer_by_id(lid)]
        if not valid_ids:
            return None

        group_layer = Layer(name=group_name, layer_type="group")
        group_layer.children_ids = valid_ids

        # Set parent_id on child layers
        for lid in valid_ids:
            lyr = self.get_layer_by_id(lid)
            if lyr:
                lyr.parent_id = group_layer.id

        # Insert group layer above top child
        top_idx = max(self.get_layer_index(lid) for lid in valid_ids)
        self.layers.insert(top_idx + 1, group_layer)
        self.active_layer_ids = [group_layer.id]
        self.notify_changed()
        return group_layer

    def ungroup_layer(self, group_id: str):
        group_lyr = self.get_layer_by_id(group_id)
        if not group_lyr or group_lyr.layer_type != "group":
            return
        for cid in group_lyr.children_ids:
            child = self.get_layer_by_id(cid)
            if child:
                child.parent_id = None
        self.remove_layers([group_id])

    # Canvas Resizing
    def set_canvas_size(self, width: int, height: int, anchor: str = "Center"):
        if width <= 0 or height <= 0:
            return
        dx = (width - self.canvas_width) / 2.0
        dy = (height - self.canvas_height) / 2.0

        self.canvas_width = width
        self.canvas_height = height

        # Adjust layer offsets based on anchor
        if anchor == "Center":
            for lyr in self.layers:
                lyr.offset_x += dx
                lyr.offset_y += dy
        self.notify_changed()

    # Mask Updates & History
    def update_mask(self, new_mask: np.ndarray, layer_id: Optional[str] = None, description: str = "Edit Mask"):
        target_layer = self.get_layer_by_id(layer_id) if layer_id else self.active_layer
        if target_layer is None:
            return

        old_mask = target_layer.mask.copy() if target_layer.mask is not None else None
        cmd = MaskEditCommand(self, old_mask=old_mask, new_mask=new_mask, layer_id=target_layer.id, description=description)
        self.undo_stack.push(cmd)

    def add_change_listener(self, callback: Callable):
        self._change_listeners.append(callback)

    def notify_changed(self):
        for cb in self._change_listeners:
            try:
                cb()
            except Exception as e:
                logger.error(f"Error notifying Document change listener: {e}")

    def width(self) -> int:
        return self.canvas_width

    def height(self) -> int:
        return self.canvas_height

