cmake_minimum_required(VERSION 3.22)
project(ImageCut VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Qt 6 SDK (installed via aqtinstall) ──────────────────────────────
set(QT6_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Qt/6.7.2/msvc2019_64")
set(CMAKE_PREFIX_PATH "${QT6_DIR}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ── OpenCV (pre-built SDK) ───────────────────────────────────────────
set(OPENCV_SDK "${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv_sdk/opencv/build")
set(OPENCV_INC "${OPENCV_SDK}/include")
set(OPENCV_LIB "${OPENCV_SDK}/x64/vc16/lib/opencv_world4100.lib")

# ── Sources ──────────────────────────────────────────────────────────
set(MINIZ_SOURCES
    third_party/miniz/miniz.c
    third_party/miniz/miniz.h
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/Settings.cpp
    src/utils/ImageUtils.cpp
)

set(CORE_SOURCES
    src/core/Layer.cpp
    src/core/MaskProcessor.cpp
    src/core/History.cpp
    src/core/ImageDocument.cpp
    src/core/ProjectManager.cpp
)

set(PROCESSING_SOURCES
    src/processing/ColorAdjust.cpp
    src/processing/Compositor.cpp
    src/processing/AlignUtils.cpp
    src/processing/CropTransform.cpp
)

set(AI_SOURCES
    src/ai/ColorKeyEngine.cpp
    src/ai/ModelManager.cpp
    src/ai/ONNXEngine.cpp
)

set(TOOLS_SOURCES
    src/tools/BrushTool.cpp
    src/tools/MagicWandTool.cpp
    src/tools/LassoTool.cpp
    src/tools/CropTool.cpp
    src/tools/SelectTool.cpp
)

set(WORKERS_SOURCES
    src/workers/InferenceWorker.cpp
    src/workers/BatchWorker.cpp
    src/workers/ExportWorker.cpp
    include/workers/InferenceWorker.h
    include/workers/BatchWorker.h
    include/workers/ExportWorker.h
)

set(UI_SOURCES
    src/ui/CanvasView.cpp
    src/ui/ToolBarPanel.cpp
    src/ui/TopBarPanel.cpp
    src/ui/panels/LayerPanel.cpp
    src/ui/panels/TransformPanel.cpp
    src/ui/panels/MaskPanel.cpp
    src/ui/panels/ImagePanel.cpp
    src/ui/panels/BackgroundPanel.cpp
    src/ui/panels/ToolPropertiesPanel.cpp
    src/ui/dialogs/ExportDialog.cpp
    src/ui/dialogs/BatchDialog.cpp
    src/ui/dialogs/SettingsDialog.cpp
    src/ui/Style.cpp
    src/ui/MainWindow.cpp
    include/ui/CanvasView.h
    include/ui/ToolBarPanel.h
    include/ui/TopBarPanel.h
    include/ui/panels/LayerPanel.h
    include/ui/panels/TransformPanel.h
    include/ui/panels/MaskPanel.h
    include/ui/panels/ImagePanel.h
    include/ui/panels/BackgroundPanel.h
    include/ui/panels/ToolPropertiesPanel.h
    include/ui/dialogs/ExportDialog.h
    include/ui/dialogs/BatchDialog.h
    include/ui/dialogs/SettingsDialog.h
    include/ui/MainWindow.h
)

add_executable(ImageCut
    src/main.cpp
    ${MINIZ_SOURCES}
    ${UTILS_SOURCES}
    ${CORE_SOURCES}
    ${PROCESSING_SOURCES}
    ${AI_SOURCES}
    ${TOOLS_SOURCES}
    ${WORKERS_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(ImageCut PRIVATE
    include
    ${OPENCV_INC}
)

target_compile_definitions(ImageCut PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(ImageCut PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OPENCV_LIB}
)

if(MSVC)
    target_compile_options(ImageCut PRIVATE /W3 /EHsc /Zc:__cplusplus)
endif()
