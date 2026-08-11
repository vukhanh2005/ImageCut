import numpy as np
from PySide6.QtCore import QThread, Signal
from app.ai.onnx_engine import ONNXModelEngine
from app.ai.color_key_engine import ColorKeyEngine
from app.utils.logger import logger

class BackgroundRemovalWorker(QThread):
    """
    Asynchronous worker thread to execute AI background removal
    without freezing the PySide6 main GUI thread.
    """
    progress = Signal(int)           # 0 to 100
    finished = Signal(np.ndarray)    # Emits generated alpha mask
    error = Signal(str)              # Emits error message string

    def __init__(self, image: np.ndarray, engine_type: str = "AI", model_name: str = "RMBG-1.4", device: str = "Auto", key_color=None, tolerance=40):
        super().__init__()
        self.image = image
        self.engine_type = engine_type  # "AI" or "ColorKey"
        self.model_name = model_name
        self.device = device
        self.key_color = key_color
        self.tolerance = tolerance

    def run(self):
        try:
            self.progress.emit(10)

            if self.engine_type == "ColorKey":
                engine = ColorKeyEngine()
                self.progress.emit(50)
                mask = engine.process(self.image, key_color=self.key_color, tolerance=self.tolerance)
                self.progress.emit(100)
                self.finished.emit(mask)
                return

            # AI Engine
            engine = ONNXModelEngine()
            self.progress.emit(25)

            if not engine.is_loaded() or engine._current_model_name != self.model_name:
                loaded = engine.load(model_name=self.model_name, device=self.device)
                if not loaded:
                    logger.warning("Could not load AI ONNX model. Falling back to classical segmentation.")

            self.progress.emit(60)
            mask = engine.process(self.image)
            self.progress.emit(100)

            self.finished.emit(mask)
        except Exception as e:
            logger.error(f"Error in background removal worker: {e}", exc_info=True)
            self.error.emit(str(e))
