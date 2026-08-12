# ImageCut - Professional Background Removal Tool

A native C++ desktop application for professional image background removal, built with **Qt 6** and **OpenCV 4**.

## Features

- **AI Auto Remove Background** - One-click AI-powered background removal using GrabCut
- **Manual Tools** - Brush, Eraser, Magic Wand, Freehand Lasso, Polygonal Lasso
- **Smart Snapping** - Magnet alignment guides for precise layer positioning
- **Multi-Layer Support** - Full layer management with opacity, visibility, blending
- **Transform Tools** - Move, Scale, Rotate, Flip with aspect ratio lock
- **Undo/Redo** - Full history tracking for all operations
- **Export** - PNG, JPEG, BMP, WebP with quality settings
- **Batch Processing** - Process multiple images at once

## Tech Stack

- **Language**: C++17
- **GUI Framework**: Qt 6.7.2 (MSVC 64-bit)
- **Image Processing**: OpenCV 4.10.0
- **Build System**: CMake 3.20+
- **Compiler**: MSVC 2022 (Visual Studio 18)

## Project Structure

```
ImageCut/
├── CMakeLists.txt          # Build configuration
├── include/                # Header files
│   ├── ai/                 # AI/ML engines
│   ├── core/               # Core data structures (Layer, Document, History)
│   ├── processing/         # Image processing utilities
│   ├── tools/              # Tool implementations
│   ├── ui/                 # UI components (Canvas, Panels, Dialogs)
│   ├── utils/              # Utilities (Logger, Settings, ImageUtils)
│   └── workers/            # Background thread workers
├── src/                    # Source files (mirrors include/)
├── installer/              # Inno Setup installer script
├── third_party/            # Third-party libraries (miniz)
├── lib/                    # Pre-built libraries
└── build/                  # Build output (generated)
```

## Building

### Prerequisites
- Visual Studio 2022 (MSVC toolchain)
- CMake 3.20+
- Qt 6.7.2 (MSVC 64-bit)
- OpenCV 4.10.0

### Build Commands
```bash
cmake -B build -G "Visual Studio 18 2025" -A x64
cmake --build build --config Release
```

### Creating Installer
```bash
# Requires Inno Setup 6
iscc installer/ImageCut.iss
# Output: installer/output/ImageCut_Setup_v1.0.0.exe
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| H | Select & Move Tool |
| B | Restore Brush |
| E | Eraser |
| W | Magic Wand |
| L | Freehand Lasso |
| P | Polygonal Lasso |
| C | Crop |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+0 | Fit to View |
| Ctrl+1 | Zoom 100% |
| Delete | Delete Layer |

## License

MIT License - NVK Studio
