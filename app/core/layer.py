import uuid
from typing import List, Optional, Tuple
import numpy as np

class Layer:
    """
    Represents an independent layer in the multi-layer compositing document.
    Types: 'image', 'text', 'shape', 'group'.
    """

    def __init__(
        self,
        name: str = "Layer",
        image: Optional[np.ndarray] = None,
        layer_type: str = "image",
        opacity: float = 1.0,
        visible: bool = True,
        locked: bool = False,
        blend_mode: str = "Normal",
        layer_id: Optional[str] = None
    ):
        self.id: str = layer_id or str(uuid.uuid4())
        self.name: str = name
        self.layer_type: str = layer_type  # "image", "text", "shape", "group"
        self.image: Optional[np.ndarray] = image  # RGB or RGBA uint8 ndarray
        self.mask: Optional[np.ndarray] = None   # uint8 ndarray (0..255) monochrome alpha mask

        self.opacity: float = opacity       # 0.0 to 1.0
        self.visible: bool = visible       # True / False
        self.locked: bool = locked         # True / False
        self.blend_mode: str = blend_mode   # Normal, Multiply, Screen, Overlay, Darken, Lighten, Add, Difference

        # Transform Properties
        self.offset_x: float = 0.0
        self.offset_y: float = 0.0
        self.scale_x: float = 1.0
        self.scale_y: float = 1.0
        self.lock_aspect: bool = True
        self.rotation: float = 0.0          # degrees (-360 to 360)
        self.flip_h: bool = False
        self.flip_v: bool = False

        # Color Adjustments per layer
        self.brightness: int = 0      # -100 to 100
        self.contrast: int = 0        # -100 to 100
        self.saturation: int = 0      # -100 to 100
        self.exposure: int = 0        # -100 to 100
        self.temperature: int = 0     # -100 to 100
        self.sharpness: int = 0       # 0 to 100

        # Mask Post-processing Options per layer
        self.feather_radius: float = 0.0
        self.smooth_kernel: int = 0
        self.expand_contract_val: int = 0
        self.edge_contrast: float = 1.0
        self.decontaminate: bool = False

        # Grouping & Hierarchy
        self.parent_id: Optional[str] = None
        self.children_ids: List[str] = []   # For group layers

        # Text Properties (for layer_type == "text")
        self.text_content: str = "Sample Text"
        self.font_family: str = "Arial"
        self.font_size: int = 48
        self.font_bold: bool = False
        self.font_italic: bool = False
        self.text_color: Tuple[int, int, int] = (255, 255, 255)

        # Shape Properties (for layer_type == "shape")
        self.shape_type: str = "Rectangle" # "Rectangle", "Circle", "Line"
        self.fill_color: Tuple[int, int, int, int] = (0, 120, 215, 255)
        self.stroke_color: Tuple[int, int, int, int] = (255, 255, 255, 255)
        self.stroke_width: int = 2

        # Cache & Performance Optimization Flags
        self._cached_rgba: Optional[np.ndarray] = None
        self._dirty: bool = True

    def invalidate_cache(self):
        """Marks the layer texture cache as dirty so it will be re-processed."""
        self._dirty = True
        self._cached_rgba = None


    def width(self) -> int:
        if self.image is not None:
            return self.image.shape[1]
        return 200

    def height(self) -> int:
        if self.image is not None:
            return self.image.shape[0]
        return 200

    def copy(self) -> "Layer":
        new_layer = Layer(
            name=f"{self.name} copy",
            image=self.image.copy() if self.image is not None else None,
            layer_type=self.layer_type,
            opacity=self.opacity,
            visible=self.visible,
            locked=self.locked,
            blend_mode=self.blend_mode
        )
        if self.mask is not None:
            new_layer.mask = self.mask.copy()

        new_layer.offset_x = self.offset_x
        new_layer.offset_y = self.offset_y
        new_layer.scale_x = self.scale_x
        new_layer.scale_y = self.scale_y
        new_layer.lock_aspect = self.lock_aspect
        new_layer.rotation = self.rotation
        new_layer.flip_h = self.flip_h
        new_layer.flip_v = self.flip_v

        new_layer.brightness = self.brightness
        new_layer.contrast = self.contrast
        new_layer.saturation = self.saturation
        new_layer.exposure = self.exposure
        new_layer.temperature = self.temperature
        new_layer.sharpness = self.sharpness

        new_layer.feather_radius = self.feather_radius
        new_layer.smooth_kernel = self.smooth_kernel
        new_layer.expand_contract_val = self.expand_contract_val
        new_layer.edge_contrast = self.edge_contrast
        new_layer.decontaminate = self.decontaminate

        new_layer.parent_id = self.parent_id
        new_layer.children_ids = list(self.children_ids)

        new_layer.text_content = self.text_content
        new_layer.font_family = self.font_family
        new_layer.font_size = self.font_size
        new_layer.font_bold = self.font_bold
        new_layer.font_italic = self.font_italic
        new_layer.text_color = self.text_color

        new_layer.shape_type = self.shape_type
        new_layer.fill_color = self.fill_color
        new_layer.stroke_color = self.stroke_color
        new_layer.stroke_width = self.stroke_width
        return new_layer

