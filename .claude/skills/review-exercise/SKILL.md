---
name: review-exercise
description: Review bài tập C của người học (build, chạy, soi theo pattern và lỗi embedded), chỉ ra lỗi và lý do mà không viết lại hộ. Dùng khi người học nói "review", "chấm bài", "xem code tôi", "/review-exercise <file hoặc thư mục>".
---

# Review bài tập

Tham số: file hoặc thư mục. Không có → dùng thư mục bài học đang mở / file thay đổi gần nhất (`git status`).

Nguyên tắc: **dạy chứ không làm hộ.** Chỉ ra lỗi, giải thích tại sao sai, gợi ý hướng sửa. Chỉ viết lại toàn bộ khi người học yêu cầu rõ ràng. Không sửa file của người học.

## Các bước

1. **Đọc đề bài** trong `LECTURE.md` / `README.md` của bài đó để biết yêu cầu.

2. **Build với cờ nghiêm ngặt** (không cần Makefile của họ):
   `gcc -std=c99 -Wall -Wextra -Werror -pedantic -Iinc src/*.c main.c -o <scratchpad>/review.exe`
   Build thêm một lần với `-fsanitize=address,undefined` nếu toolchain hỗ trợ (UCRT64 có thể không hỗ trợ ASan — nếu lỗi link thì bỏ qua và nói rõ).
   Chạy chương trình, ghi lại output thật.

3. **Soi code theo 3 lớp**, mỗi lỗi ghi `file:dòng`:

   **a. Đúng yêu cầu đề bài?** Thiếu hàm nào, hành vi nào sai.

   **b. Đúng pattern đang học?** Dùng checklist trong mục "Tóm tắt và checklist" của `LECTURE.md`. Với Object Pattern ví dụ: còn biến `static` có state không, `self` có ở tham số đầu, có `init`/`deinit`, `init` có xóa sạch struct, hàm chỉ đọc có `const`.

   **c. Lỗi embedded C kinh điển:**
   - Integer overflow trong phép kiểm tra biên (`a + b > MAX`)
   - Tràn buffer, off-by-one, `strcpy`/`sprintf` không giới hạn
   - Dùng `int`/`long` thay vì `<stdint.h>` khi kích thước quan trọng
   - Thiếu `volatile` cho biến dùng chung với ISR hoặc thanh ghi phần cứng
   - Signed/unsigned so sánh lẫn nhau, shift trên số có dấu
   - Không kiểm tra giá trị trả về
   - Magic number không có tên
   - Header thiếu include guard hoặc expose thứ không cần

4. **Trả kết quả** bằng tiếng Việt theo mẫu:

   ```
   ## Kết quả build & chạy
   (lệnh, cảnh báo/lỗi, output)

   ## 🔴 Lỗi phải sửa
   1. file:dòng — lỗi gì → tại sao sai → gợi ý hướng sửa (không đưa code hoàn chỉnh)

   ## 🟡 Nên cải thiện
   ...

   ## ✅ Làm tốt
   (cụ thể, không khen chung chung)

   ## Câu hỏi cho bạn
   1–2 câu hỏi buộc người học suy nghĩ về lựa chọn thiết kế của chính họ.
   ```

5. **Ghi điểm yếu** vào `PROGRESS.md` (mục "Điểm yếu cần ôn") nếu phát hiện lỗi hiểu sai khái niệm, không phải lỗi gõ nhầm.
