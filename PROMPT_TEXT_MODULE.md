# TASK PROMPT: Tự động Clone Dự án, Tạo Nhánh temp1, Phân tích Kiến trúc & Tối ưu hóa Module Text (C++ / Qt6 / OpenCV)

## 🚀 BƯỚC 1: LỆNH THỰC THI THIẾT LẬP DỰ ÁN (GIT WORKFLOW)
Trước khi làm bất kỳ công việc nào, hãy chạy chuỗi lệnh terminal sau để tải mã nguồn chính thức và chuyển sang nhánh làm việc tạm thời `temp1`:

```bash
# 1. Clone repository về máy
git clone https://github.com/vukhanh2005/ImageCut.git
cd ImageCut

# 2. Tạo và chuyển sang nhánh tạm temp1 để làm việc an toàn
git checkout -b temp1
```

---

## 🎯 BƯỚC 2: MỤC TIÊU CÔNG VIỆC (OBJECTIVE)
Bạn là một Chuyên gia Kỹ sư Phần mềm C++ Native & Đồ họa UI/UX. Nhiệm vụ của bạn là đọc hiểu toàn bộ kiến trúc dự án **ImageCut** (một ứng dụng Desktop đồ họa tách nền & biên tập ảnh chuyên nghiệp viết bằng Pure C++20, Qt 6.7.2, OpenCV 4.10.0 và CMake), sau đó tập trung phân tích, khắc phục lỗi và tối ưu hóa **Module Text (Văn bản nghệ thuật & Paragraph Text Box)**.

---

## 🛠️ CÔNG NGHỆ & MÔ HÌNH THIẾT KẾ (TECH STACK & ARCHITECTURE)
- **Ngôn ngữ**: Pure C++20 (MSVC 2019/2022 64-bit). Không sử dụng Python.
- **Framework GUI**: Qt 6.7.2 (Core, Gui, Widgets).
- **Xử lý hình ảnh**: OpenCV 4.10.0 (`cv::Mat` ARGB32 / RGBA8888).
- **Hệ thống Layer**:
  - `ImageDocument`: Quản lý danh sách các Layer (`std::vector<std::shared_ptr<Layer>>`).
  - `Layer`: Đại diện cho 1 lớp ảnh (`image`), lớp văn bản (`text`), hoặc lớp hình vẽ (`shape`).
  - `Compositor`: Engine blend layer và render Text/Shape thành `cv::Mat`.

---

## 📁 CÁC FILE CHÍNH LIÊN QUAN ĐẾN MODULE TEXT NÊN PHÂN TÍCH:
Hãy đọc và kiểm tra kỹ các file nguồn sau đây trước khi chỉnh sửa:

1. **Model & Lưu trữ dữ liệu Layer**:
   - `include/core/Layer.h`
   - `src/core/Layer.cpp`
   *(Chứa các thuộc tính: `textContent`, `fontFamily`, `fontSize`, `fontBold`, `fontItalic`, `textColor`, `textHasStroke`, `textStrokeColor`, `textStrokeWidth`, `textHasShadow`, `textShadowColor`, `textShadowOffsetX`, `textShadowOffsetY`, `textHasBg`, `textBgColor`, `textWrapWidth`, `textAlignment`)*.

2. **Engine Render Văn Bản (Compositor Engine)**:
   - `src/processing/Compositor.cpp` (hàm `renderTextLayer`)
   *(Xử lý đo đạc font metrics bằng `QFontMetrics`, tự động ngắt dòng `Qt::TextWordWrap`, vẽ stroke viền chữ `QPainterPath`, vẽ bóng đổ shadow, vẽ khung nền pill background và convert sang `cv::Mat (CV_8UC4)`)*.

3. **Giao diện tương tác & Kéo giãn khung (Canvas View & Handles)**:
   - `src/ui/CanvasView.cpp`
   - `include/ui/CanvasView.h`
   *(Xử lý sự kiện chuột `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, tính toán khung chọn `getLayerScreenPolygon`, 8 nấc resize handles, xoay rot handle, và cập nhật `textWrapWidth` theo thời gian thực khi kéo viền)*.

4. **Bảng điều khiển thuộc tính (Right Sidebar Properties Panel)**:
   - `include/ui/panels/ObjectPropertiesPanel.h`
   - `src/ui/panels/ObjectPropertiesPanel.cpp`
   *(Tab **Properties** bên phải giúp chỉnh sửa nội dung Text, Font Family, Font Size, Bold/Italic, Căn lề Left/Center/Right, Wrap Width, Màu chữ với Live Preview & Cancel Restore)*.

5. **Lưu/Mở dự án `.icproj`**:
   - `src/core/ProjectManager.cpp`
   *(Serialize/Deserialize tất cả thuộc tính Text sang JSON/Base64 để bảo toàn khi lưu/mở lại dự án)*.

---

## 🎯 YÊU CẦU NGHIỆM THU MODULE TEXT (TEXT MODULE REQUIREMENTS):

1. **Cơ chế Paragraph Text Box (Khung chữ tự động ngắt dòng)**:
   - Khi `textWrapWidth == 0`: Text ở chế độ Single Line Point Text (tự động tính chiều rộng theo chuỗi chữ).
   - Khi `textWrapWidth > 0`: Text ở chế độ Paragraph Text Box.
   - Khi kéo nấc resize (Handles) bên trái/phải của Text Layer:
     - Tăng/giảm `textWrapWidth` tương ứng với chiều rộng mới của khung.
     - **Nếu kéo rộng khung**: Chữ ở dòng dưới phải tự động nhảy lên điền đầy dòng trên.
     - **Nếu kéo hẹp khung**: Chữ tự động ngắt xuống dòng tiếp theo (`Qt::TextWordWrap`).
     - **Giữ cố định `scaleX = 1.0` và `scaleY = 1.0`** để font chữ không bị co dãn méo hay vỡ pixel.

2. **Xem trước màu thời gian thực (Live Color Preview & Cancel Restore)**:
   - Khi chọn màu chữ (`textColor`), màu viền (`textStrokeColor`), hay màu nền (`textBgColor`) bằng `QColorDialog`:
     - Phải phát sự kiện `currentColorChanged` để hiển thị màu mới trực tiếp trên Canvas theo thời gian thực.
     - Nếu người dùng nhấn **Cancel** (hoặc tắt hộp thoại), phải khôi phục lại đúng màu ban đầu (`rejected` signal).

3. **Căn lề (Text Alignment)**:
   - Hỗ trợ căn trái (`Qt::AlignLeft`), căn giữa (`Qt::AlignHCenter`), và căn phạm (`Qt::AlignRight`).

---

## 🔍 QUY TRÌNH THỰC HIỆN DÀNH CHO AI AGENT:
1. Chạy các lệnh Git ở **BƯỚC 1** để lấy mã nguồn và tạo nhánh `temp1`.
2. Phân tích codebase và các file cấu trúc liên quan ở BƯỚC 2.
3. Tiến hành tinh chỉnh / khắc phục các lỗi đọng lại nếu có.
4. Biên dịch dự án bằng CMake & MSVC Release (`cmake --build build --config Release`) và báo cáo kết quả chi tiết.
