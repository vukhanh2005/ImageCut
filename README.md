# Personal Background Remover & Image Editor

A desktop application for AI-powered automatic background removal, interactive mask editing, smart selection, background replacement, non-destructive editing, and high-resolution export on Windows.

---

## Key Features

- **AI Automatic Background Removal**: High-precision segmentation using local ONNX Runtime inference (`RMBG-1.4`, `U2Net`, `Silueta`) with CPU and GPU acceleration. Runs asynchronously on background threads to keep the UI 100% responsive.
- **Manual Mask Refinement**:
  - **Restore Brush (B)**: Paint on canvas to restore missing foreground details.
  - **Eraser (E)**: Paint to remove remaining background regions.
  - Custom brush size, hardness, opacity, and feathering.
  - Dynamic live brush cursor preview.
- **Smart Selection & Tools**:
  - **Magic Wand (W)**: Smart flood-fill color selection using CIE LAB distance with configurable tolerance.
  - **Lasso Tool (L)**: Polygon and freehand selection outlines.
  - **Interactive Crop (C)**: Bounding box crop handles with 1:1, 4:3, 16:9, and freeform aspect ratios.
  - **Select & Pan (H / Space)**: Drag canvas viewport or move foreground layers.
- **Edge Refinement & Matting**:
  - Gaussian Feathering, Bilateral Smoothing, Morphological Expansion/Contraction (Dilate/Erode), Edge Contrast adjustment, and Color Decontamination for hair/clothing halos.
- **Background Replacement & Blur**:
  - **Transparent**: Checkerboard background preview for PNG alpha channel.
  - **Solid Color**: Quick color presets and custom `QColorDialog` picker.
  - **Custom Image**: Import background images.
  - **Gradient**: Custom dual-color linear gradients.
  - **Background Blur**: Independent 0-100% Gaussian blur applied strictly to background while keeping foreground crisp.
- **Non-Destructive Adjustments**:
  - Brightness, Contrast, Saturation, Exposure, Color Temperature, and Sharpness controls.
- **Undo / Redo History**: Full command-pattern history stack (`Ctrl+Z` / `Ctrl+Shift+Z`).
- **Project Save & Load (`.bgrem`)**: Dedicated project file format storing base image, mask, layer settings, adjustments, and metadata into a zip container.
- **Batch Processing**: Dedicated multi-file / directory background removal queue.
- **High-Resolution Export**: Export PNG (transparent), JPG (quality 1-100), WEBP (lossy/lossless), with aspect-ratio locked resizing.

---

## 🛠️ Technology Stack

- **GUI Framework**: PySide6 (Qt 6 for Python)
- **AI Inference Engine**: ONNX Runtime (`onnxruntime` CPU / DirectML / CUDA)
- **Image Processing**: OpenCV (`opencv-python`), Pillow (PIL), NumPy, SciPy
- **Packaging**: PyInstaller (`build_exe.py`)

---

## 🚀 Installation & Setup

### Requirements
- Windows 10/11 x64
- Python 3.10+ installed

### Step 1: Clone or Download Workspace
Navigate to the project root directory:
```bash
cd d:\NVK\NVK\Projects\ImageCut
```

### Step 2: Install Dependencies
```bash
pip install -r requirements.txt
```

### Step 3: Run the Application
```bash
python main.py
```

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Description |
|---|---|
| `Ctrl+O` | Open Image File |
| `Ctrl+S` | Save Project (`.bgrem`) |
| `Ctrl+E` | Export Final Image |
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `B` | Activate Restore Brush Tool |
| `E` | Activate Eraser Tool |
| `W` | Activate Magic Wand Tool |
| `L` | Activate Lasso Selection Tool |
| `C` | Activate Interactive Crop Tool |
| `H` / `Space` | Hand / Pan Canvas Viewport |
| `Ctrl+0` | Fit Canvas in Viewport |
| `Ctrl+1` | Zoom 100% |

---

## 📦 Building Standalone `.exe`

To package the application into a single standalone Windows executable `dist/BackgroundRemover.exe`:

```bash
python build_exe.py
```

---

## 🧪 Testing

Run the automated unit and integration test suite:
```bash
pytest -v
```
