# Current Features Inventory — ImageCut Project

The following table lists **all existing, actual features** verified in the Python reference implementation of `ImageCut`, along with their corresponding implementation files and migration priority for the C++ version.

---

## Feature List

| Feature | Description | Existing Implementation Files | Migration Priority |
|---|---|---|---|
| **Multi-Layer Support** | Add, remove, duplicate, group/ungroup, hide/show, lock/unlock independent image layers | `app/core/layer.py`, `app/core/image_document.py`, `app/ui/panels/layer_panel.py` | **High** |
| **Z-Order Management** | Reorder layers: Move to Top, Move Up, Move Down, Move to Bottom, Drag & Drop list | `app/core/image_document.py`, `app/ui/panels/layer_panel.py` | **High** |
| **Layer Transforms** | Offset X/Y, Scale X/Y, Lock Aspect Ratio, Rotation (-360°..360°), Flip Horizontal, Flip Vertical | `app/core/layer.py`, `app/processing/crop_transform.py`, `app/ui/panels/transform_panel.py` | **High** |
| **8-Handle Bounding Box** | Interactive canvas resize handles (8 corners/sides + 1 top rotation handle) for active layer transform | `app/ui/canvas.py` (`get_layer_screen_polygon`, `get_handle_at_point`, `mouseMoveEvent`) | **High** |
| **AI Background Removal** | High-precision segmentation using local ONNX models (`RMBG-1.4`, `U2Net`, `Silueta`) with CPU/CUDA | `app/ai/onnx_engine.py`, `app/ai/model_manager.py`, `app/workers/inference_worker.py` | **High** |
| **Chroma Key Removal** | Color-distance segmentation engine in CIE LAB color space with tolerance and feathering | `app/ai/color_key_engine.py` | **Medium** |
| **Classical GrabCut Fallback** | Automated fallback segmentation using OpenCV `cv2.grabCut` when AI models are unavailable | `app/ai/onnx_engine.py` (`_fallback_classical_remove`) | **Medium** |
| **Restore Brush Tool** | Paint on canvas to restore missing foreground details (diameter, line interpolation) | `app/tools/brush_tool.py` (`mode="Restore"`) | **High** |
| **Eraser Tool** | Paint on canvas to remove remaining background regions | `app/tools/brush_tool.py` (`mode="Eraser"`) | **High** |
| **Magic Wand Tool** | Smart flood-fill color selection using OpenCV `cv2.floodFill` or LAB distance with tolerance | `app/tools/magic_wand_tool.py` | **High** |
| **Lasso Selection Tool** | Freehand / polygon selection outline using OpenCV `cv2.fillPoly` to keep or remove regions | `app/tools/lasso_tool.py` | **High** |
| **Interactive Crop Tool** | Visual crop bounding box, rule-of-thirds grid, aspect ratio locks (Free, 1:1, 16:9, 4:3) | `app/tools/crop_tool.py`, `app/processing/crop_transform.py` | **High** |
| **Select & Pan Tool** | Drag canvas viewport or move foreground layers directly with mouse | `app/tools/select_tool.py`, `app/ui/canvas.py` | **High** |
| **Edge Refinement Filters** | Gaussian Feathering, Bilateral/Median Smoothing, Morphological Expand/Contract (Dilate/Erode) | `app/core/mask.py` (`MaskProcessor`) | **High** |
| **Color Decontamination** | Inpainting (`cv2.inpaint` TELEA) on semi-transparent edge pixels to remove hair/clothing background halos | `app/core/mask.py` (`decontaminate_colors`) | **Medium** |
| **Mask View Modes** | Switch canvas preview mode between Normal, Red Overlay, Black & White Mask, and Alpha Channel | `app/core/image_document.py`, `app/processing/compositing.py`, `app/ui/panels/mask_panel.py` | **High** |
| **Color Adjustments** | Non-destructive Brightness, Contrast, Saturation (HSV), Exposure, Temperature, Sharpness | `app/processing/color_adjust.py`, `app/ui/panels/image_panel.py` | **High** |
| **Blend Modes** | 8 layer blend modes: Normal, Multiply, Screen, Overlay, Darken, Lighten, Add, Difference | `app/processing/compositing.py` (`blend_layer_onto_canvas_roi`) | **High** |
| **Background Replacement** | Transparent (checkerboard grid), Solid Color (with picker & presets), Custom Image, Gradient | `app/processing/compositing.py`, `app/ui/panels/background_panel.py` | **High** |
| **Background Blur** | Independent 0..50px Gaussian blur applied strictly to background while keeping foreground sharp | `app/processing/compositing.py`, `app/ui/panels/background_panel.py` | **Medium** |
| **Align & Distribute** | Align selected layers to Canvas or Selection (Left, Center, Right, Top, Middle, Bottom) & Distribute H/V | `app/processing/align_utils.py`, `app/ui/panels/transform_panel.py` | **Medium** |
| **Text Layers** | Vector text layer rendering with custom string, font family, font size, bold, italic, and color | `app/core/layer.py`, `app/processing/compositing.py` (`render_text_layer`) | **Low** |
| **Shape Layers** | Vector shape layer rendering (Rectangle, Circle) with fill color, stroke color, and stroke width | `app/core/layer.py`, `app/processing/compositing.py` (`render_shape_layer`) | **Low** |
| **Undo / Redo History** | Command-pattern history stack (`Ctrl+Z` / `Ctrl+Shift+Z`) up to 30 depth | `app/core/history.py` (`UndoStack`, `MaskEditCommand`) | **High** |
| **Project Save & Load** | Native `.bgrem` zip container serialization storing metadata, layer images, masks, and background | `app/core/project.py` (`ProjectManager`) | **High** |
| **High-Res Export** | Export to PNG (transparent), JPG (quality 1-100), WEBP with custom dimensions & social media presets | `app/ui/dialogs/export_dialog.py`, `app/workers/export_worker.py` | **High** |
| **Batch Processing** | Asynchronous queue worker processing folders of images to remove backgrounds automatically | `app/ui/dialogs/batch_dialog.py`, `app/workers/batch_worker.py` | **High** |
| **Drag & Drop Import** | Drag single or multiple image files directly from Windows File Explorer onto canvas view | `app/ui/canvas.py` (`dragEnterEvent`, `dropEvent`) | **High** |
| **Grid & Snapping** | Show customizable grid lines and snap layers to grid/canvas edges | `app/core/image_document.py`, `app/ui/canvas.py`, `app/ui/panels/transform_panel.py` | **Medium** |
| **Viewport Zoom & Pan** | Smooth mouse wheel zoom anchored under cursor, fit in view (`Ctrl+0`), 100% zoom (`Ctrl+1`), status bar | `app/ui/canvas.py`, `app/ui/main_window.py` | **High** |
| **Dark Theme UI** | Sleek modern dark mode UI stylesheet with high contrast, cyan accents, and status indicator badges | `app/ui/style.py` | **High** |
| **Settings & Preferences** | Persist AI default model, hardware target (Auto/CUDA/CPU), export quality, and theme in JSON file | `app/utils/settings.py`, `app/ui/dialogs/settings_dialog.py` | **High** |
