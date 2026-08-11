from abc import ABC, abstractmethod
import numpy as np

class BackgroundRemovalEngine(ABC):
    """
    Abstract base class for all background removal engines.
    Allows easy swapping between AI model engines, Color-key engines, and manual engines.
    """
    
    @abstractmethod
    def load(self, model_name: str = "RMBG-1.4", device: str = "Auto") -> bool:
        """Loads the model or resources into memory."""
        pass

    @abstractmethod
    def unload(self):
        """Unloads the model resources from memory."""
        pass

    @abstractmethod
    def process(self, image: np.ndarray, **kwargs) -> np.ndarray:
        """
        Processes an input image (RGB or RGBA uint8 ndarray)
        and returns an alpha mask (grayscale uint8 ndarray [0..255]).
        """
        pass

    @abstractmethod
    def is_loaded(self) -> bool:
        """Returns whether the model is currently loaded."""
        pass
