import os
import json
from app.utils.logger import logger

DEFAULT_SETTINGS = {
    "ai_model": "RMBG-1.4",       # Choices: "RMBG-1.4" (High Quality), "U2Net" (Balanced), "Silueta" (Fast)
    "ai_device": "Auto",           # Choices: "Auto", "CUDA", "CPU"
    "inference_size": 512,         # Choices: 320, 512, 1024
    "theme": "Dark",               # Choices: "Dark", "Light"
    "checkerboard_size": 16,       # Canvas transparency grid size
    "export_format": "PNG",        # Choices: "PNG", "JPG", "WEBP"
    "export_quality": 95,          # 1-100
    "export_folder": "",           # Default empty (prompt user)
    "max_undo_steps": 30,          # Undo stack size
    "recent_files": []
}

class AppSettings:
    """Manages application settings persisted in a local JSON config file."""
    
    def __init__(self, config_path: str = None):
        if config_path is None:
            config_dir = os.path.join(os.path.expanduser("~"), ".imagecut")
            os.makedirs(config_dir, exist_ok=True)
            config_path = os.path.join(config_dir, "settings.json")
            
        self.config_path = config_path
        self.data = DEFAULT_SETTINGS.copy()
        self.load()

    def load(self):
        """Loads settings from config file if exists."""
        if os.path.exists(self.config_path):
            try:
                with open(self.config_path, "r", encoding="utf-8") as f:
                    loaded = json.load(f)
                    self.data.update(loaded)
                logger.info(f"Settings loaded from {self.config_path}")
            except Exception as e:
                logger.error(f"Error loading settings file: {e}. Resetting to defaults.")

    def save(self):
        """Saves current settings to config file."""
        try:
            os.makedirs(os.path.dirname(self.config_path), exist_ok=True)
            with open(self.config_path, "w", encoding="utf-8") as f:
                json.dump(self.data, f, indent=2, ensure_ascii=False)
            logger.info(f"Settings saved to {self.config_path}")
        except Exception as e:
            logger.error(f"Error saving settings: {e}")

    def get(self, key: str, default=None):
        return self.data.get(key, default if default is not None else DEFAULT_SETTINGS.get(key))

    def set(self, key: str, value):
        self.data[key] = value
        self.save()

# Global settings instance
settings = AppSettings()
