# C++ Migration Plan — ImageCut Project

This document outlines the phase-by-phase engineering migration plan to port the existing Python `ImageCut` desktop application to C++ inside `Cpp/`.

---

## Migration Architecture Overview

```text
Existing Python Architecture
        ↓
Phase 1: Project Setup (CMake, Directory Tree, Qt6 / OpenCV / ONNX Linking)
        ↓
Phase 2: Core Data Models (Layer, ImageDocument, MaskProcessor)
        ↓
Phase 3: Image Processing & Compositing Engine (ColorAdjust, ROI Affine Compositor)
        ↓
Phase 4: Undo / Redo Command Pattern (UndoStack, MaskEditCommand)
        ↓
Phase 5: Interactive GUI Canvas (CanvasView, 8-Handle Transform, Overlays)
        ↓
Phase 6: GUI Controls & Panels (LayerPanel, TransformPanel, MaskPanel, ImagePanel, BGPanel)
        ↓
Phase 7: Interactive Tool System (Brush, Eraser, Magic Wand, Lasso, Crop, Select)
        ↓
Phase 8: AI Inference Engine (ONNX Runtime C++ Integration & ModelManager)
        ↓
Phase 9: Project File Serialization (.bgrem Zip / nlohmann_json)
        ↓
Phase 10: Export & Batch Processing Workers (Asynchronous QThreads)
        ↓
Phase 11: Main Window, Stylesheet, & Application Assembly
        ↓
Phase 12: Automated Unit Testing (GoogleTest) & Verification
```

---

## Detailed Phase Breakdown

### Phase 1: Project Setup & Build Infrastructure

* **Goal**: Establish C++ workspace structure, CMake configuration, and third-party library dependencies.
* **Target Files**:
  * [NEW] `Cpp/CMakeLists.txt`
  * [NEW] `Cpp/README.md`
  * [NEW] `Cpp/include/`
  * [NEW] `Cpp/src/`
  * [NEW] `Cpp/tests/`
  * [NEW] `Cpp/third_party/`
* **Dependencies**: Qt 6.7+, OpenCV 4.x, CMake 3.22+, MSVC C++20 compiler.
* **Verification**: Build empty Qt6 application target executable `ImageCut.exe` successfully with `cmake --build build --config Release`.

---

### Phase 2: Core Data Models

* **Goal**: Port `Layer`, `ImageDocument`, and `MaskProcessor` classes with zero GUI dependency.
* **Source Reference**:
  * [app/core/layer.py](file:///d:/NVK/NVK/Projects/ImageCut/app/core/layer.py)
  * [app/core/image_document.py](file:///d:/NVK/NVK/Projects/ImageCut/app/core/image_document.py)
  * [app/core/mask.py](file:///d:/NVK/NVK/Projects/ImageCut/app/core/mask.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/core/Layer.h` / `Cpp/src/core/Layer.cpp`
  * [NEW] `Cpp/include/core/ImageDocument.h` / `Cpp/src/core/ImageDocument.cpp`
  * [NEW] `Cpp/include/core/MaskProcessor.h` / `Cpp/src/core/MaskProcessor.cpp`
* **Key Tasks**:
  * Store layer image & mask as `cv::Mat`.
  * Implement layer coordinate mapping helper `mapCanvasPosToLayerPos(QPointF, Layer*)`.
  * Implement `MaskProcessor` static methods: `feather`, `smooth`, `expandContract`, `adjustEdgeContrast`, `decontaminateColors`.

---

### Phase 3: Image Processing & Compositing Engine

* **Goal**: Port `apply_image_adjustments` and `composite_document` with texture caching and ROI patch slicing.
* **Source Reference**:
  * [app/processing/color_adjust.py](file:///d:/NVK/NVK/Projects/ImageCut/app/processing/color_adjust.py)
  * [app/processing/compositing.py](file:///d:/NVK/NVK/Projects/ImageCut/app/processing/compositing.py)
  * [app/processing/align_utils.py](file:///d:/NVK/NVK/Projects/ImageCut/app/processing/align_utils.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/processing/ColorAdjust.h` / `Cpp/src/processing/ColorAdjust.cpp`
  * [NEW] `Cpp/include/processing/Compositor.h` / `Cpp/src/processing/Compositor.cpp`
  * [NEW] `Cpp/include/processing/AlignUtils.h` / `Cpp/src/processing/AlignUtils.cpp`
* **Key Tasks**:
  * C++ `cv::warpAffine` ROI patch warping to achieve 60+ FPS viewport interaction.
  * Implement 8 blend modes in C++ (Normal, Multiply, Screen, Overlay, Darken, Lighten, Add, Difference).

---

### Phase 4: Undo / Redo Command History

* **Goal**: Port `UndoStack`, `Command`, `MaskEditCommand`, `DocumentActionCommand`.
* **Source Reference**:
  * [app/core/history.py](file:///d:/NVK/NVK/Projects/ImageCut/app/core/history.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/core/History.h` / `Cpp/src/core/History.cpp`
* **Key Tasks**:
  * RAII `std::unique_ptr<Command>` memory management in `UndoStack`.

---

### Phase 5: Interactive GUI Canvas

* **Goal**: Port `CanvasView` (custom `QGraphicsView` & `QGraphicsScene`).
* **Source Reference**:
  * [app/ui/canvas.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/canvas.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/ui/CanvasView.h` / `Cpp/src/ui/CanvasView.cpp`
* **Key Tasks**:
  * Render cyan canvas boundary, amber gold corners, and dimension tag in `drawForeground`.
  * Calculate 8 transform handles + 1 rotation handle in screen coordinates.
  * Implement mouse drag resizing, rotation, and move logic.

---

### Phase 6: GUI Controls & Panels

* **Goal**: Port sidebar control tabs (`LayerManagerPanel`, `TransformPanel`, `MaskPanel`, `ImagePanel`, `BackgroundPanel`).
* **Source Reference**:
  * [app/ui/panels/layer_panel.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/panels/layer_panel.py)
  * [app/ui/panels/transform_panel.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/panels/transform_panel.py)
  * [app/ui/panels/mask_panel.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/panels/mask_panel.py)
  * [app/ui/panels/image_panel.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/panels/image_panel.py)
  * [app/ui/panels/background_panel.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/panels/background_panel.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/ui/panels/...` & `Cpp/src/ui/panels/...`

---

### Phase 7: Interactive Tool System

* **Goal**: Port tool classes (`BaseTool`, `MaskBrushTool`, `MagicWandTool`, `LassoTool`, `CropTool`, `SelectMoveTool`).
* **Source Reference**:
  * [app/tools/base_tool.py](file:///d:/NVK/NVK/Projects/ImageCut/app/tools/base_tool.py)
  * [app/tools/brush_tool.py](file:///d:/NVK/NVK/Projects/ImageCut/app/tools/brush_tool.py)
  * [app/tools/magic_wand_tool.py](file:///d:/NVK/NVK/Projects/ImageCut/app/tools/magic_wand_tool.py)
  * [app/tools/lasso_tool.py](file:///d:/NVK/NVK/Projects/ImageCut/app/tools/lasso_tool.py)
  * [app/tools/crop_tool.py](file:///d:/NVK/NVK/Projects/ImageCut/app/tools/crop_tool.py)
  * [app/tools/select_tool.py](file:///d:/NVK/NVK/Projects/ImageCut/app/tools/select_tool.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/tools/...` & `Cpp/src/tools/...`

---

### Phase 8: AI Inference Engine

* **Goal**: Port `ONNXModelEngine` and `ModelManager` to native ONNX Runtime C++ API (`onnxruntime_cxx_api.h`).
* **Source Reference**:
  * [app/ai/onnx_engine.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ai/onnx_engine.py)
  * [app/ai/model_manager.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ai/model_manager.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/ai/ONNXEngine.h` / `Cpp/src/ai/ONNXEngine.cpp`
  * [NEW] `Cpp/include/ai/ModelManager.h` / `Cpp/src/ai/ModelManager.cpp`

---

### Phase 9: Project Serialization (`.bgrem`)

* **Goal**: Port `ProjectManager` save and load functionality using `nlohmann_json` and `miniz`.
* **Source Reference**:
  * [app/core/project.py](file:///d:/NVK/NVK/Projects/ImageCut/app/core/project.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/core/ProjectManager.h` / `Cpp/src/core/ProjectManager.cpp`

---

### Phase 10: Export & Batch Workers

* **Goal**: Port asynchronous `ExportWorker`, `BatchWorker`, and `InferenceWorker` using `QThread`.
* **Source Reference**:
  * [app/workers/export_worker.py](file:///d:/NVK/NVK/Projects/ImageCut/app/workers/export_worker.py)
  * [app/workers/batch_worker.py](file:///d:/NVK/NVK/Projects/ImageCut/app/workers/batch_worker.py)
  * [app/workers/inference_worker.py](file:///d:/NVK/NVK/Projects/ImageCut/app/workers/inference_worker.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/include/workers/...` & `Cpp/src/workers/...`

---

### Phase 11: Main Window & Final Application Assembly

* **Goal**: Assemble `MainWindow`, `TopBarPanel`, `ToolBarPanel`, menu bar shortcuts, dark QSS stylesheet, and `main.cpp` entry point.
* **Source Reference**:
  * [app/ui/main_window.py](file:///d:/NVK/NVK/Projects/ImageCut/app/ui/main_window.py)
  * [main.py](file:///d:/NVK/NVK/Projects/ImageCut/main.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/src/main.cpp`
  * [NEW] `Cpp/src/ui/MainWindow.cpp`

---

### Phase 12: Automated Testing & Verification

* **Goal**: Port all Python pytest tests to GoogleTest C++ suite.
* **Source Reference**:
  * [tests/test_document.py](file:///d:/NVK/NVK/Projects/ImageCut/tests/test_document.py)
  * [tests/test_multi_layer.py](file:///d:/NVK/NVK/Projects/ImageCut/tests/test_multi_layer.py)
  * [tests/test_ai_engine.py](file:///d:/NVK/NVK/Projects/ImageCut/tests/test_ai_engine.py)
* **C++ Module Implementation**:
  * [NEW] `Cpp/tests/test_document.cpp`
  * [NEW] `Cpp/tests/test_compositor.cpp`
  * [NEW] `Cpp/tests/test_project.cpp`
