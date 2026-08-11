import os
import urllib.request
import numpy as np
from app.utils.logger import logger

MODEL_CONFIGS = {
    "RMBG-1.4": {
        "url": "https://huggingface.co/briaai/RMBG-1.4/resolve/main/onnx/model.onnx",
        "filename": "rmbg-1.4.onnx",
        "input_size": (1024, 1024),
        "mean": [0.5, 0.5, 0.5],
        "std": [0.5, 0.5, 0.5]
    },
    "U2Net": {
        "url": "https://github.com/danielgatis/rembg/releases/download/v0.0.0/u2net.onnx",
        "filename": "u2net.onnx",
        "input_size": (320, 320),
        "mean": [0.485, 0.456, 0.406],
        "std": [0.229, 0.224, 0.225]
    },
    "Silueta": {
        "url": "https://github.com/danielgatis/rembg/releases/download/v0.0.0/silueta.onnx",
        "filename": "silueta.onnx",
        "input_size": (320, 320),
        "mean": [0.485, 0.456, 0.406],
        "std": [0.229, 0.224, 0.225]
    }
}

class ModelManager:
    """Manages downloading, caching, and loading of ONNX segmentation models."""

    def __init__(self, models_dir: str = None):
        if models_dir is None:
            models_dir = os.path.join(os.path.expanduser("~"), ".imagecut", "models")
        self.models_dir = models_dir
        os.makedirs(self.models_dir, exist_ok=True)

    def get_model_path(self, model_name: str) -> str:
        if model_name not in MODEL_CONFIGS:
            model_name = "RMBG-1.4"
        filename = MODEL_CONFIGS[model_name]["filename"]
        return os.path.join(self.models_dir, filename)

    def is_model_downloaded(self, model_name: str) -> bool:
        path = self.get_model_path(model_name)
        return os.path.exists(path) and os.path.getsize(path) > 1000

    def download_model(self, model_name: str, progress_callback=None) -> str:
        if model_name not in MODEL_CONFIGS:
            model_name = "RMBG-1.4"

        config = MODEL_CONFIGS[model_name]
        dest_path = self.get_model_path(model_name)

        if self.is_model_downloaded(model_name):
            logger.info(f"Model {model_name} already exists at {dest_path}")
            return dest_path

        logger.info(f"Downloading model {model_name} from {config['url']}...")
        temp_path = dest_path + ".tmp"

        def _reporthook(blocknum, blocksize, totalsize):
            if progress_callback and totalsize > 0:
                percent = min(100, int((blocknum * blocksize / totalsize) * 100))
                progress_callback(percent)

        try:
            req = urllib.request.Request(
                config['url'],
                headers={'User-Agent': 'Mozilla/5.0'}
            )
            with urllib.request.urlopen(req) as response, open(temp_path, 'wb') as out_file:
                total_length = response.headers.get('content-length')
                totalsize = int(total_length) if total_length else 0
                blocksize = 8192
                blocknum = 0
                while True:
                    buffer = response.read(blocksize)
                    if not buffer:
                        break
                    out_file.write(buffer)
                    blocknum += 1
                    _reporthook(blocknum, blocksize, totalsize)

            os.replace(temp_path, dest_path)
            logger.info(f"Model downloaded successfully to {dest_path}")
            return dest_path
        except Exception as e:
            if os.path.exists(temp_path):
                os.remove(temp_path)
            logger.error(f"Failed to download model {model_name}: {e}")
            raise e

model_manager = ModelManager()
