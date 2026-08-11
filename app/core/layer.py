import numpy as np

class Layer:
    """Represents a composite layer in the document (Background or Foreground)."""

    def __init__(self, name: str, image: np.ndarray = None, opacity: float = 1.0, visible: bool = True):
        self.name = name
        self.image = image            # RGB or RGBA uint8 ndarray
        self.opacity = opacity        # 0.0 to 1.0
        self.visible = visible        # True / False
        self.offset_x = 0
        self.offset_y = 0
        self.scale = 1.0
        self.rotation = 0.0           # degrees
        self.flip_h = False
        self.flip_v = False

    def copy(self):
        new_layer = Layer(
            name=self.name,
            image=self.image.copy() if self.image is not None else None,
            opacity=self.opacity,
            visible=self.visible
        )
        new_layer.offset_x = self.offset_x
        new_layer.offset_y = self.offset_y
        new_layer.scale = self.scale
        new_layer.rotation = self.rotation
        new_layer.flip_h = self.flip_h
        new_layer.flip_v = self.flip_v
        return new_layer
