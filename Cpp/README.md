# ImageCut — Native C++ Version

A native C++ desktop application for AI-powered automatic background removal, interactive mask editing, multi-layer image compositing, non-destructive color adjustments, and high-resolution batch export on Windows x64.

---

## 🛠️ Technology Stack

* **Language**: C++20
* **GUI Framework**: Qt 6.7+ (`Qt6::Widgets`, `Qt6::Gui`, `Qt6::Core`)
* **Image Processing**: OpenCV 4.x (`opencv_core`, `opencv_imgproc`, `opencv_imgcodecs`)
* **AI Inference**: ONNX Runtime C++ API (`onnxruntime`)
* **Build System**: CMake 3.22+ & Visual Studio MSVC

---

## 📁 Directory Structure

```text
Cpp/
├── CMakeLists.txt              # CMake build script
├── README.md                   # C++ version documentation
├── include/                    # Header files
│   ├── ai/                     # ONNX & ColorKey engines
│   ├── core/                   # ImageDocument, Layer, MaskProcessor, History, ProjectManager
│   ├── processing/             # Compositor, ColorAdjust, AlignUtils, CropTransform
│   ├── tools/                  # Brush, Eraser, MagicWand, Lasso, Crop, Select
│   ├── ui/                     # CanvasView, MainWindow, Panels & Dialogs
│   ├── utils/                  # Logger, Settings, ImageUtils
│   └── workers/                # Export, Batch, and Inference QThreads
├── src/                        # Implementation files
│   └── main.cpp                # Application entry point
├── third_party/                # Miniz & JSON helpers
└── docs/                       # Project audit & migration plans
```

---

## 🚀 How to Build

### Requirements
* Windows 10/11 x64
* Visual Studio 2022 / MSVC C++20
* Qt 6.7+ installed
* OpenCV 4.x installed

### Build Steps

```bash
cd Cpp
cmake -S . -B build
cmake --build build --config Release
```

Run the built executable:
```bash
./build/Release/ImageCut.exe
```
