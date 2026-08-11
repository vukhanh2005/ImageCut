from typing import List, Callable, Tuple
import numpy as np
import cv2
from app.core.history import UndoStack, MaskEditCommand
from app.core.layer import Layer
from app.utils.logger import logger

class ImageDocument:
    """
    Central document manager representing an open image session.
    Maintains raw original image, alpha mask, composite layers, background options,
    image adjustments, crop rectangle, and non-destructive rendering.
    """

    def __init__(self, original_image: np.ndarray = None):
        self.undo_stack = UndoStack()

        # Raw original image (RGB or RGBA uint8 ndarray) - NEVER MODIFIED DIRECTLY
        self._original_image: np.ndarray = None

        # Alpha mask (H, W) uint8 ndarray [0..255]
        self.mask: np.ndarray = None

        # Background Configuration
        self.bg_type: str = "Transparent"    # "Transparent", "Solid", "Image", "Gradient"
        self.bg_color: Tuple[int, int, int] = (255, 255, 255) # RGB
        self.bg_color_end: Tuple[int, int, int] = (0, 0, 0)   # For gradient
        self.bg_image: np.ndarray = None     # Custom imported background image
        self.bg_blur: int = 0                # 0 to 100 px
        self.bg_opacity: float = 1.0          # 0.0 to 1.0
        self.bg_offset_x: int = 0
        self.bg_offset_y: int = 0
        self.bg_scale: float = 1.0
        self.bg_rotation: float = 0.0

        # Mask Display & Post-processing options
        self.mask_view_mode: str = "Normal"  # "Normal", "Overlay", "BlackWhite", "Alpha"
        self.mask_opacity: float = 0.5       # For Overlay mode
        self.feather_radius: float = 0.0
        self.smooth_kernel: int = 0
        self.expand_contract_val: int = 0
        self.edge_contrast: float = 1.0
        self.decontaminate: bool = False

        # Image Color Adjustments
        self.brightness: int = 0      # -100 to 100
        self.contrast: int = 0        # -100 to 100
        self.saturation: int = 0      # -100 to 100
        self.exposure: int = 0        # -100 to 100
        self.temperature: int = 0     # -100 to 100
        self.sharpness: int = 0       # 0 to 100

        # Transform & Crop
        self.crop_rect: Tuple[int, int, int, int] = None # (x, y, w, h)
        self.fg_offset_x: int = 0
        self.fg_offset_y: int = 0
        self.fg_scale: float = 1.0
        self.fg_rotation: float = 0.0
        self.fg_flip_h: bool = False
        self.fg_flip_v: bool = False

        # Event listeners
        self._change_listeners: List[Callable] = []

        if original_image is not None:
            self.set_original_image(original_image)

    @property
    def original_image(self) -> np.ndarray:
        return self._original_image

    def set_original_image(self, image: np.ndarray):
        """Sets a new base image and resets/initializes alpha mask."""
        if image is None or image.size == 0:
            return

        if image.shape[2] == 4:
            # Extract initial alpha if present
            initial_alpha = image[:, :, 3].copy()
            self._original_image = image[:, :, :3].copy()
        else:
            self._original_image = image.copy()
            initial_alpha = np.full((image.shape[0], image.shape[1]), 255, dtype=np.uint8)

        self.mask = initial_alpha
        self.crop_rect = (0, 0, image.shape[1], image.shape[0])
        self.undo_stack.clear()
        self.notify_changed()

    def update_mask(self, new_mask: np.ndarray, description: str = "Edit Mask"):
        """Pushes a new mask update to history stack."""
        if self.mask is None:
            self.mask = new_mask.copy()
            self.notify_changed()
            return

        cmd = MaskEditCommand(self, old_mask=self.mask, new_mask=new_mask, description=description)
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
        return self._original_image.shape[1] if self._original_image is not None else 0

    def height(self) -> int:
        return self._original_image.shape[0] if self._original_image is not None else 0
