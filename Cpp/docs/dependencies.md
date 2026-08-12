# C++ Dependency Audit & Strategy — ImageCut

This document evaluates the dependencies of the existing Python application, maps them to their equivalent native C++ replacements, and defines the setup and compatibility strategy for the MSVC / Windows build environment.

---

## Dependency Mapping Table

| Current Python Dependency | Purpose in Existing App | Target C++ Replacement | Minimal Version | C++ Library Provider / Integration Strategy |
|---|---|---|---|---|
| **PySide6** (Qt 6.6+ for Python) | Desktop GUI, Main Window, Custom Canvas GraphicsView, Panels, Signals/Slots | **Qt 6 C++** (`Qt6Widgets`, `Qt6Gui`, `Qt6Core`) | Qt 6.5+ (Qt 6.7+ Recommended) | Installed via Official Qt Online Installer or MSVC vcpkg (`vcpkg install qtbase:x64-windows`) |
| **opencv-python** (OpenCV 4.8+) | Matrix operations, `warpAffine`, color space conversion (`cvtColor`), `floodFill`, `inpaint`, `GaussianBlur`, `dilate`/`erode` | **OpenCV C++** (`opencv_core`, `opencv_imgproc`, `opencv_imgcodecs`) | OpenCV 4.8+ | System OpenCV build or CMake `find_package(OpenCV REQUIRED)` / vcpkg (`vcpkg install opencv4:x64-windows`) |
| **onnxruntime** (1.16+) | Running background removal AI segmentation models (`RMBG-1.4`, `U2Net`, `Silueta`) on CPU / CUDA / DirectML | **ONNX Runtime C++ API** (`onnxruntime.dll` / `onnxruntime_cxx_api.h`) | ONNX Runtime 1.16+ | Direct NuGet / ONNX Runtime official C++ release package or vcpkg (`onnxruntime:x64-windows`) |
| **Pillow (PIL)** (10.0+) | Image decoding/encoding for export & loading formats (PNG, JPG, WEBP, BMP) | **OpenCV `cv::imread` / `cv::imencode`** & **Qt `QImage`** | N/A | Included within OpenCV and Qt 6 core modules (supports PNG, JPG, WEBP, BMP, TIFF natively) |
| **NumPy** (1.24+) | Multi-dimensional array storage & fast slicing for RGBA buffers & masks | **OpenCV `cv::Mat`** & **`std::vector<uint8_t>`** | N/A | `cv::Mat` provides zero-copy continuous uint8 array storage, channel split/merge, ROI header slicing (`Mat(rect)`), and direct buffer access |
| **SciPy** (1.11+) | Gaussian filter for mask feathering (`scipy.ndimage.gaussian_filter`) | **OpenCV `cv::GaussianBlur`** | N/A | Standard `cv::GaussianBlur` with `sigmaX = sigmaY = radius` |
| **zipfile & json** (Python stdlib) | Saving and opening native `.bgrem` project container files (ZIP archive containing `project.json` + layer PNG assets) | **`nlohmann_json`** & **`libzip`** (or Qt `QZipReader` / `QZipWriter` / `miniz`) | `nlohmann_json` 3.11+, `miniz` 3.0+ | `nlohmann_json` single header + `miniz.c` (header-only / single file lightweight zip support) |
| **pytest** (7.4+) | Automated unit & integration testing | **GoogleTest (gtest)** | 1.14+ | FetchContent in CMake or vcpkg (`gtest:x64-windows`) |

---

## Detailed Evaluation & Compatibility Strategy

### 1. GUI Framework: Qt 6 C++ (`Qt6Widgets`)
* **Compatibility**: 100% direct feature parity with PySide6. PySide6 is just a Python wrapper over Qt 6 C++ headers.
* **Key Components to Port**:
  * `QMainWindow` -> `src/ui/MainWindow.h/.cpp`
  * `QGraphicsView` / `QGraphicsScene` -> `src/ui/CanvasView.h/.cpp`
  * Signals and Slots mechanism (`Q_OBJECT`, `emit`, `connect`)
  * Qt Layouts (`QVBoxLayout`, `QHBoxLayout`, `QGridLayout`, `QTabWidget`)
  * Qt Controls (`QSlider`, `QComboBox`, `QSpinBox`, `QDoubleSpinBox`, `QPushButton`, `QListWidget`)

### 2. Image Processing: OpenCV C++ (`cv::Mat`)
* **Compatibility**: 100% feature parity. All Python `cv2` calls map 1:1 to C++ `cv::` namespace functions.
* **Key Mappings**:
  * `np.ndarray` (H, W, C) `uint8` -> `cv::Mat` (type `CV_8UC3` or `CV_8UC4` or `CV_8UC1`).
  * `cv2.cvtColor(...)` -> `cv::cvtColor(src, dst, cv::COLOR_BGR2RGB)`.
  * `cv2.warpAffine(...)` -> `cv::warpAffine(src, dst, M, dsize, flags, borderMode, borderValue)`.
  * `cv2.floodFill(...)` -> `cv::floodFill(image, mask, seedPoint, newVal, rect, loDiff, upDiff, flags)`.
  * `cv2.inpaint(...)` -> `cv::inpaint(src, inpaintMask, dst, inpaintRadius, cv::INPAINT_TELEA)`.

### 3. AI Inference: ONNX Runtime C++ API
* **Compatibility**: High performance, zero Python overhead.
* **Header**: `#include <onnxruntime_cxx_api.h>`
* **C++ Pipeline**:
  1. `Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ImageCut");`
  2. `Ort::SessionOptions session_options;`
  3. `session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);`
  4. Enable CUDA or DirectML if available in C++ session.
  5. `Ort::Session session(env, model_path.c_str(), session_options);`
  6. Create input tensor `Ort::Value::CreateTensor<float>(...)` from preprocessed float vector.
  7. Run inference: `session.Run(Ort::RunOptions{nullptr}, input_node_names.data(), &input_tensor, 1, output_node_names.data(), 1);`

### 4. Project Serialization (`.bgrem`)
* **JSON**: Use `nlohmann::json` to parse/serialize `project.json` format version 2.
* **ZIP Archive**: Use `miniz` (single file `miniz.h`/`miniz.c`) or `libzip` to write `project.json`, `layer_{i}_img.png`, and `layer_{i}_mask.png` into `.bgrem` archives.

---

## Build System & CMake Configuration

The C++ project root `Cpp/` will use **CMake 3.22+** as its primary build system with full support for Microsoft Visual Studio MSVC on Windows.

```cmake
cmake_minimum_required(VERSION 3.22)
project(ImageCut VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt 6
find_package(Qt6 COMPONENTS Widgets Gui Core REQUIRED)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(include third_party)
```
