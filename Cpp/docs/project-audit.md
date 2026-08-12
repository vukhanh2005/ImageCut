# Project Audit — ImageCut (Desktop Image Editor & Background Remover)

> **Document Version**: 1.0.0  
> **Date**: August 12, 2026  
> **Source Project**: `ImageCut` (Python 3.10+, PySide6, OpenCV, ONNX Runtime)  
> **Target Project Location**: `Cpp/`  

---

## 1. Project Overview

The current project is a desktop application named **ImageCut** (also referenced as **BackgroundRemover**), designed for AI-powered automatic background removal, manual mask refinement, multi-layer image compositing, non-destructive color adjustments, edge matting/feathering, background replacement/blur, interactive cropping/alignment, and high-resolution batch export on Windows x64.

The project is structured around a central non-destructive document model supporting multi-layer Z-ordering, ROI patch warp compositing, real-time texture caching, and undo/redo command history.

---

## 2. Existing Technology Stack

* **Programming Language**: Python 3.10+
* **GUI Framework**: PySide6 6.6+ (Qt 6 bindings for Python)
* **AI Inference Engine**: `onnxruntime` 1.16+ (Supporting CPU, DirectML, and CUDA execution providers)
* **Image Processing**: OpenCV (`opencv-python` 4.8+), Pillow (PIL 10.0+), NumPy (1.24+), SciPy (1.11+)
* **Packaging**: PyInstaller 6.0+ (`build_exe.py` / `BackgroundRemover.spec`)
* **Testing**: `pytest` 7.4+ (`tests/`)

---

## 3. Project Structure

```text
ImageCut/
├── app/
│   ├── ai/
│   │   ├── base.py                   # Abstract BackgroundRemovalEngine base class
│   │   ├── color_key_engine.py       # Chroma key / LAB distance segmentation engine
│   │   ├── model_manager.py          # Model downloader, config dictionary, HuggingFace/GitHub caching
│   │   └── onnx_engine.py            # ONNX Runtime inference engine with GrabCut/Ellipse fallback
│   ├── core/
│   │   ├── history.py                # Command pattern UndoStack, MaskEditCommand, DocumentActionCommand
│   │   ├── image_document.py         # Central Document model (Layers, Canvas, View settings, Map coordinates)
│   │   ├── layer.py                  # Layer model (Image, Text, Shape, Group, Transform, Adjustments, Cache)
│   │   ├── mask.py                   # MaskProcessor (Feather, Smooth, Expand/Contract, Contrast, Decontaminate)
│   │   └── project.py                # ProjectManager (.bgrem Zip container save/load serialization)
│   ├── processing/
│   │   ├── align_utils.py            # Align & Distribute layer utility functions
│   │   ├── color_adjust.py           # Brightness, Contrast, Saturation, Exposure, Temp, Sharpness
│   │   ├── compositing.py            # composite_document, ROI patch warp affine & 8 blend modes
│   │   └── crop_transform.py         # Bounding box crop & Affine transform helpers
│   ├── tools/
│   │   ├── base_tool.py              # Abstract BaseTool class
│   │   ├── brush_tool.py             # Restore Brush & Eraser mask painting tool
│   │   ├── crop_tool.py              # Interactive Crop rectangle & Rule-of-thirds overlay tool
│   │   ├── lasso_tool.py             # Freehand/polygon selection lasso tool
│   │   ├── magic_wand_tool.py        # Magic Wand LAB/OpenCV floodFill color selection tool
│   │   └── select_tool.py            # Select & Move canvas pan tool
│   ├── ui/
│   │   ├── dialogs/
│   │   │   ├── batch_dialog.py       # Batch background removal queue dialog
│   │   │   ├── export_dialog.py      # High-resolution export configuration dialog
│   │   │   └── settings_dialog.py    # Preferences dialog (AI model, hardware device, theme)
│   │   ├── panels/
│   │   │   ├── background_panel.py   # Background replacement (Solid, Image, Gradient, Blur) tab
│   │   │   ├── image_panel.py        # Color & Tone adjustment sliders tab
│   │   │   ├── layer_panel.py        # Layer manager list, Z-order, Opacity, Blend Modes, Grouping tab
│   │   │   ├── mask_panel.py         # Mask view mode & edge refinement sliders tab
│   │   │   └── transform_panel.py   # Transform coordinates, Alignment, Canvas size presets tab
│   │   ├── canvas.py                 # Interactive QGraphicsView canvas with handles & overlays
│   │   ├── main_window.py            # Main QMainWindow layout, Menu bar, Shortcuts, Status bar
│   │   ├── style.py                  # Dark QSS theme stylesheet
│   │   ├── toolbar.py                # Left vertical tool selector panel
│   │   └── top_bar.py                # Top quick actions bar & AI progress bar
│   ├── utils/
│   │   ├── image_utils.py            # NumPy <-> QImage/QPixmap/PIL conversion & load_image
│   │   ├── logger.py                 # File & Console Logger
│   │   └── settings.py               # JSON settings manager (`~/.imagecut/settings.json`)
│   └── workers/
│       ├── batch_worker.py           # QThread for multi-file folder batch processing
│       ├── export_worker.py          # QThread for high-res export rendering
│       └── inference_worker.py       # QThread for async AI model background removal
├── tests/                            # PyTest unit & integration tests
├── Cpp/                              # TARGET C++ PROJECT ROOT (Strict isolation)
├── main.py                           # Application entry point
├── build_exe.py                      # PyInstaller script
└── requirements.txt                  # Python dependencies
```

---

## 4. GUI Architecture

* **Framework**: PySide6 (`QMainWindow`, `QGraphicsView`, `QGraphicsScene`).
* **Canvas Component (`CanvasView`)**:
  * Inherits from `QGraphicsView`.
  * Manages viewport zooming (`zoom_factor`, wheel event anchored under mouse, shortcuts `Ctrl++`, `Ctrl+-`, `Ctrl+0`, `Ctrl+1`).
  * Manages spacebar panning (`HandCursor`).
  * Implements interactive transform handles: 8 resize handles (`tl`, `tc`, `tr`, `ml`, `mr`, `bl`, `bc`, `br`) + 1 rotation handle (`rot`), drawn dynamically in `drawForeground`.
  * Renders neon cyan canvas border, amber gold corner brackets, and floating canvas dimension badge (`Canvas: W × H px`).
  * Supports drag-and-drop image import directly into canvas (`dragEnterEvent`, `dropEvent`).
  * Communicates hit testing (`get_layer_at_point`) to select layers by clicking on canvas.
* **Control Panels & Tab Widget**:
  * `LayerManagerPanel`: Displays layer stack list (`QListWidget`), Z-order reordering via drag & drop, eye visibility button, lock button, thumbnail, opacity slider (0..100%), blend mode dropdown (8 modes), context menu.
  * `TransformPanel`: X, Y, Scale X, Scale Y, Lock Aspect Ratio, Rotation (-360°..360°), Flip H, Flip V, Alignment buttons (Left, Center, Right, Top, Middle, Bottom), Distribution buttons (Horizontal, Vertical), Canvas Presets & Resize.
  * `MaskPanel`: Mask view mode dropdown (Normal, Overlay, BlackWhite, Alpha), Feathering, Smoothness, Expand/Contract, Edge Contrast, Color Decontamination.
  * `ImagePanel`: Brightness, Contrast, Saturation, Exposure, Temperature, Sharpness sliders + Reset button.
  * `BackgroundPanel`: Background type (Transparent, Solid, Image, Gradient), Color pickers, Preset quick colors, BG Image import, Background Blur slider (0..50px).
* **Dialogs**: `ExportDialog`, `BatchDialog`, `SettingsDialog`.

---

## 5. Image Processing Pipeline

* **Data Representation**: NumPy `uint8` arrays. Color images are HWC (3 channels RGB or 4 channels RGBA). Alpha masks are single-channel grayscale `uint8` (0..255, where 255 = fully opaque foreground, 0 = fully transparent background).
* **Color Adjustments** (`apply_image_adjustments`):
  1. Brightness & Exposure: linear float offset.
  2. Contrast: sigmoidal factor around 128.0 midpoint.
  3. Saturation: RGB -> HSV conversion, scaling S channel, HSV -> RGB.
  4. Temperature: Warm (+Red, -Blue) or Cool (+Blue, -Red) shift.
  5. Sharpness: Unsharp masking via `cv2.GaussianBlur` and `cv2.addWeighted`.
* **Compositing Engine** (`composite_document`):
  1. Generate base canvas background (Solid color, Linear gradient, Custom background image with blur, or Transparent).
  2. For each visible layer (Z-order bottom-to-top):
     - Compute layer texture RGBA (color adjustments + mask refinement filters + caching).
     - Compute 4 rotated corner coordinates in canvas space to find a tight ROI bounding box `(xmin, xmax, ymin, ymax)`.
     - Construct 3x3 affine transformation matrix combining translation, scaling, rotation, and center offset.
     - Warp layer RGBA slice into ROI patch using `cv2.warpAffine`.
     - Blend ROI patch into accumulated canvas RGBA buffer using alpha blending formula for selected blend mode (Normal, Multiply, Screen, Overlay, Darken, Lighten, Add, Difference).

---

## 6. AI / Background Removal

* **Model Management** (`ModelManager`):
  * Caches models in `~/.imagecut/models/`.
  * Automatically downloads models from HuggingFace / GitHub releases with HTTP progress callback.
* **Supported ONNX Segmentation Models**:
  1. `RMBG-1.4`: BriaAI RMBG 1.4 model (Input: 1024×1024, Mean: [0.5,0.5,0.5], Std: [0.5,0.5,0.5]). High precision matting.
  2. `U2Net`: U^2-Net model (Input: 320×320, Mean: [0.485,0.456,0.406], Std: [0.229,0.224,0.225]). Balanced performance.
  3. `Silueta`: Silueta model (Input: 320×320, Mean: [0.485,0.456,0.406], Std: [0.229,0.224,0.225]). Ultra fast.
* **Inference Pipeline** (`ONNXModelEngine`):
  * Image preprocessing: RGB standardization -> Resize to model target input size -> Normalize float32 `(img - mean) / std` -> HWC to NCHW transpose.
  * ONNX Session Execution: `SessionOptions` graph optimization enabled, providers: `CUDAExecutionProvider`, `DmlExecutionProvider`, `CPUExecutionProvider`.
  * Postprocessing: Squeeze tensor -> Sigmoid activation (if values outside 0..1) -> Multiply 255.0 & clip uint8 -> Resize back to original layer dimensions.
  * Fallback Mechanism: Classical OpenCV `cv2.grabCut` initialized with margin rectangle if ONNX fails or is unavailable.
* **Color Key Engine** (`ColorKeyEngine`):
  * Chroma key background removal using Euclidean distance in CIE LAB color space (`cv2.COLOR_RGB2LAB`) with configurable tolerance and boundary feathering.

---

## 7. Layer System

* Layer Class (`Layer`):
  * Attributes: `id` (UUID), `name`, `layer_type` ("image", "text", "shape", "group"), `image` (RGB array), `mask` (uint8 alpha mask array).
  * State: `opacity` (0.0..1.0), `visible` (bool), `locked` (bool), `blend_mode` (string).
  * Transform: `offset_x`, `offset_y`, `scale_x`, `scale_y`, `lock_aspect`, `rotation` (-360°..360°), `flip_h`, `flip_v`.
  * Adjustments: `brightness`, `contrast`, `saturation`, `exposure`, `temperature`, `sharpness`.
  * Mask Options: `feather_radius`, `smooth_kernel`, `expand_contract_val`, `edge_contrast`, `decontaminate`.
  * Grouping: `parent_id`, `children_ids`.
  * Text Props: `text_content`, `font_family`, `font_size`, `font_bold`, `font_italic`, `text_color`.
  * Shape Props: `shape_type` ("Rectangle", "Circle"), `fill_color`, `stroke_color`, `stroke_width`.
  * Caching: `_cached_rgba`, `_dirty` flag for 60+ FPS viewport interaction.

---

## 8. Mask System

* Mask Representation: Grayscale `uint8` matrix matching local layer image dimensions.
* Post-Processing Filters (`MaskProcessor`):
  * `feather`: `scipy.ndimage.gaussian_filter`.
  * `smooth`: `cv2.medianBlur`.
  * `expand_contract`: Morphological `cv2.dilate` (expand) or `cv2.erode` (contract) with ellipse structuring element (`cv2.MORPH_ELLIPSE`).
  * `adjust_edge_contrast`: Contrast multiplication around 0.5 center.
  * `decontaminate_colors`: Color decontamination using OpenCV Telea inpainting (`cv2.inpaint`) on edge transition pixels to remove background halos from hair/clothing.

---

## 9. Canvas / Rendering

* Viewport transform matrix maps canvas coordinates to scene.
* `map_canvas_pos_to_layer_pos` maps screen canvas coordinate to local layer pixel space by un-rotating around center, un-scaling, translating origin, and un-flipping.
* Tool Overlays: Brush circle indicator, Magic Wand target crosshair, Lasso marching ants polyline, Crop dimming mask with rule-of-thirds grid.

---

## 10. Undo / Redo

* `UndoStack` with max depth 30.
* Command Pattern: `Command` abstract class.
  * `MaskEditCommand`: Saves `old_mask` and `new_mask` copies.
  * `DocumentActionCommand`: Uses callable lambdas for document state mutations.

---

## 11. Project Save / Load

* Dedicated format: `.bgrem` (ZIP container with `ZIP_DEFLATED`).
* Container contents:
  * `project.json`: Full document metadata, canvas dimensions, background settings, view settings, array of layer metadata dicts.
  * `layer_{i}_img.png`: PNG image asset for each layer with image content.
  * `layer_{i}_mask.png`: PNG mask asset for each layer.
  * `bg_image.png`: Custom background image (if present).
* Full backwards compatibility with v1 single-image `.bgrem` files.

---

## 12. Import / Export

* **Import**: PNG, JPG, JPEG, WEBP, BMP, TIFF. Single or multi-image drop / select.
* **Export** (`ExportWorker`): High-res composite rendering -> Resize to target dimensions (Presets or custom) -> Output as PNG (transparent/opaque), JPG (quality 1..100, custom background color fill), or WEBP (lossy/lossless quality 1..100).

---

## 13. Batch Processing

* `BatchDialog` and `BatchWorker` (QThread).
* Supports adding individual files or entire folder trees recursively.
* Configurable output directory, AI model selection, and target output format (PNG/JPG/WEBP).

---

## 14. Settings

* Persistent JSON configuration stored in `~/.imagecut/settings.json`.
* Controls default AI model, hardware execution device (Auto/CUDA/CPU), UI theme (Dark/Light), default export settings, and undo stack depth.

---

## 15. Dependencies

| Python Dependency | Current Purpose | C++ Equivalent Replacement |
|---|---|---|
| `PySide6` (Qt6) | GUI Window, Widgets, GraphicsView, Signals/Slots | Qt 6.7+ C++ (`Qt6::Widgets`, `Qt6::Gui`, `Qt6::Core`) |
| `opencv-python` | Image transforms, matting, floodFill, inpaint, blur | OpenCV 4.x C++ (`opencv_core`, `opencv_imgproc`, `opencv_imgcodecs`) |
| `onnxruntime` | AI segmentation model execution | ONNX Runtime C++ API (`onnxruntime` 1.16+) |
| `NumPy` / `SciPy` | Matrix ops, Gaussian filtering | OpenCV `cv::Mat` & Eigen / C++ standard algorithms |
| `Pillow` (PIL) | Image loading/saving, formatting | OpenCV `cv::imread`/`cv::imencode` + `stb_image` / Qt `QImage` |
| `pytest` | Unit & integration testing framework | GoogleTest (`gtest`) |

---

## 16. Assets

* Standard Qt system cursors and Unicode emoji icon strings.
* C++ migration can utilize vector SVG icons or embedded Qt Resource system (`.qrc`).

---

## 17. Models

* ONNX Models (`RMBG-1.4`, `U2Net`, `Silueta`).
* C++ version will load identical `.onnx` model binaries from disk or download into `%USERPROFILE%/.imagecut/models/`.

---

## 18. Threading / Concurrency

* Async execution using PySide6 `QThread` workers:
  * `BackgroundRemovalWorker`: Runs ONNX model inference without freezing UI.
  * `BatchWorker`: Processes queue of images asynchronously.
  * `ExportWorker`: High-res document compositing and disk export.
* C++ target will use `QThread` or `std::async` / `QtConcurrent` for thread safety and non-blocking GUI execution.

---

## 19. Performance Characteristics

* Texture caching (`_cached_rgba`) prevents re-running layer adjustments and mask refinement filters on every frame.
* Tight ROI bounding box warp (`transform_layer_to_canvas_roi`) avoids processing offscreen / empty canvas pixels.

---

## 20. Known Problems

* Large image layers (8K+) without downscaling can consume significant RAM during undo command stack duplication.
* Color decontamination using `cv2.inpaint` can be compute-heavy on very large masks if edge region is broad.

---

## 21. Migration Risks

1. **ONNX Runtime C++ API Integration**: Managing C++ memory tensors (`Ort::Value`) and execution provider setup (CUDA/DirectML) across Windows MSVC builds.
2. **ZIP & JSON Serialization**: Ensuring `.bgrem` project files produced by C++ version are 100% binary/JSON compatible with the Python version.
3. **Graphics Scene Handles**: Exact coordinate translation for 8-handle + 1-rotation handle dragging in Qt C++ `QGraphicsView`.

---

## 22. Recommended C++ Architecture

```text
       ┌─────────────────────────────────────────┐
       │             Qt 6 GUI Layer              │
       │   MainWindow, CanvasView, ControlPanels │
       └────────────────────┬────────────────────┘
                            │
                            ▼
       ┌─────────────────────────────────────────┐
       │           Editor Controller             │
       │     ToolManager, CommandHistory (Undo)  │
       └────────────────────┬────────────────────┘
                            │
                            ▼
       ┌─────────────────────────────────────────┐
       │             Document Model              │
       │      ImageDocument, Layer, Mask         │
       └────────────────────┬────────────────────┘
                            │
                            ▼
       ┌─────────────────────────────────────────┐
       │         Processing & Rendering          │
       │    Compositor, ColorAdjust, MaskFilter  │
       └────────────────────┬────────────────────┘
                            │
             ┌──────────────┴──────────────┐
             ▼                             ▼
   ┌──────────────────┐          ┌──────────────────┐
   │  OpenCV Engine   │          │ ONNX Inference   │
   │  cv::Mat, Warp   │          │ Runtime Engine   │
   └──────────────────┘          └──────────────────┘
```
