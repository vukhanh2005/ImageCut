import os
import numpy as np
import cv2
from app.ai.base import BackgroundRemovalEngine
from app.ai.model_manager import model_manager, MODEL_CONFIGS
from app.utils.logger import logger

class ONNXModelEngine(BackgroundRemovalEngine):
    """
    ONNX Runtime background removal engine supporting CPU and CUDA execution providers.
    """

    def __init__(self):
        self._session = None
        self._current_model_name = None
        self._current_device = None

    def load(self, model_name: str = "RMBG-1.4", device: str = "Auto") -> bool:
        try:
            import onnxruntime as ort
        except ImportError:
            logger.error("onnxruntime is not installed.")
            return False

        model_path = model_manager.get_model_path(model_name)

        if not os.path.exists(model_path):
            logger.warning(f"Model file does not exist locally: {model_path}. Downloading...")
            try:
                model_manager.download_model(model_name)
            except Exception as e:
                logger.error(f"Could not download model {model_name}: {e}")
                return False

        # Determine execution providers
        available_providers = ort.get_available_providers()
        logger.info(f"Available ONNX Runtime execution providers: {available_providers}")

        providers = []
        if device in ("Auto", "CUDA") and "CUDAExecutionProvider" in available_providers:
            providers.append("CUDAExecutionProvider")
        if device in ("Auto", "DirectML") and "DmlExecutionProvider" in available_providers:
            providers.append("DmlExecutionProvider")
        providers.append("CPUExecutionProvider")

        logger.info(f"Loading ONNX session for {model_name} with providers: {providers}")
        try:
            opts = ort.SessionOptions()
            opts.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
            self._session = ort.InferenceSession(model_path, opts, providers=providers)
            self._current_model_name = model_name
            self._current_device = device
            logger.info(f"Successfully loaded ONNX model {model_name}")
            return True
        except Exception as e:
            logger.error(f"Error initializing ONNX session: {e}", exc_info=True)
            return False

    def unload(self):
        self._session = None
        self._current_model_name = None
        self._current_device = None
        logger.info("ONNX model session unloaded.")

    def is_loaded(self) -> bool:
        return self._session is not None

    def process(self, image: np.ndarray, **kwargs) -> np.ndarray:
        """
        Runs AI inference on input image ndarray (H, W, 3) or (H, W, 4) in RGB format.
        Returns alpha mask as uint8 ndarray of shape (H, W), values 0 to 255.
        """
        if image is None or image.size == 0:
            raise ValueError("Input image for background removal is empty.")

        orig_h, orig_w = image.shape[:2]

        # Standardize to RGB uint8
        if image.shape[2] == 4:
            rgb_img = cv2.cvtColor(image, cv2.COLOR_RGBA2RGB)
        else:
            rgb_img = image.copy()

        if self._session is None:
            logger.warning("ONNX session not loaded. Attempting automatic load...")
            success = self.load(model_name="RMBG-1.4", device="Auto")
            if not success:
                logger.warning("Falling back to classical GrabCut background removal.")
                return self._fallback_classical_remove(rgb_img)

        config = MODEL_CONFIGS.get(self._current_model_name, MODEL_CONFIGS["RMBG-1.4"])
        target_w, target_h = config["input_size"]

        # 1. Preprocess: Resize & Normalize
        resized = cv2.resize(rgb_img, (target_w, target_h), interpolation=cv2.INTER_LINEAR)
        img_float = resized.astype(np.float32) / 255.0

        mean = np.array(config["mean"], dtype=np.float32).reshape(1, 1, 3)
        std = np.array(config["std"], dtype=np.float32).reshape(1, 1, 3)
        normalized = (img_float - mean) / std

        # Transpose HWC -> NCHW
        tensor_in = np.transpose(normalized, (2, 0, 1))[np.newaxis, ...]

        # 2. Inference
        input_name = self._session.get_inputs()[0].name
        output_name = self._session.get_outputs()[0].name

        outputs = self._session.run([output_name], {input_name: tensor_in})
        pred = outputs[0]  # Shape: (1, 1, H, W) or (1, H, W)

        # 3. Postprocess Output
        pred_mask = np.squeeze(pred)
        if pred_mask.ndim == 3:
            pred_mask = pred_mask[0]

        # Sigmoid if values outside 0..1
        if pred_mask.min() < 0 or pred_mask.max() > 1:
            pred_mask = 1.0 / (1.0 + np.exp(-pred_mask))

        # Normalize 0..1 to 0..255 uint8
        mask_255 = (pred_mask * 255.0).clip(0, 255).astype(np.uint8)

        # Resize back to original image dimensions
        final_mask = cv2.resize(mask_255, (orig_w, orig_h), interpolation=cv2.INTER_LINEAR)
        return final_mask

    def _fallback_classical_remove(self, rgb_img: np.ndarray) -> np.ndarray:
        """Classical fallback segmentation using OpenCV GrabCut."""
        h, w = rgb_img.shape[:2]
        mask = np.zeros((h, w), np.uint8)
        bgdModel = np.zeros((1, 65), np.float64)
        fgdModel = np.zeros((1, 65), np.float64)

        # Margin rect around borders
        margin_x = max(1, int(w * 0.05))
        margin_y = max(1, int(h * 0.05))
        rect = (margin_x, margin_y, w - 2 * margin_x, h - 2 * margin_y)

        try:
            cv2.grabCut(rgb_img, mask, rect, bgdModel, fgdModel, 3, cv2.GC_INIT_WITH_RECT)
            output_mask = np.where((mask == 2) | (mask == 0), 0, 255).astype(np.uint8)
            return output_mask
        except Exception as e:
            logger.error(f"Fallback GrabCut failed: {e}")
            # Center ellipse fallback
            fallback_mask = np.zeros((h, w), dtype=np.uint8)
            cv2.ellipse(fallback_mask, (w // 2, h // 2), (w // 3, h // 3), 0, 0, 360, 255, -1)
            return fallback_mask
