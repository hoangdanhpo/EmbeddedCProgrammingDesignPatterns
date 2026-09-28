---
name: finish-lesson
description: Kết thúc một bài học pattern - kiểm tra bài tập đã làm và build được, đánh dấu ✅ trong CLAUDE.md, cập nhật PROGRESS.md và commit. Dùng khi người học nói "xong bài", "hoàn thành pattern X", "/finish-lesson [pattern]".
---

# Hoàn thành bài học

Tham số: tên pattern hoặc thư mục. Không có → bài đang học (pattern ☐ đầu tiên đã có thư mục).

## Các bước

1. **Kiểm tra điều kiện hoàn thành** — thiếu điều nào thì báo cho người học và **dừng lại**, không đánh dấu:
   - Bài tập cơ bản trong `LECTURE.md` đã có code của người học (file mới ngoài code mẫu).
   - `make clean && make run` build sạch với `-Werror` và chạy được.
   - Bài tập đã được review ít nhất một lần (hỏi người học nếu không chắc) và các lỗi 🔴 đã sửa.

2. **Hỏi 2 câu kiểm tra nhanh** về ý chính của bài. Trả lời sai cơ bản → gợi ý ôn lại mục liên quan trong `LECTURE.md` trước khi đánh dấu. Người học muốn đánh dấu luôn thì tôn trọng, nhưng ghi điểm yếu vào `PROGRESS.md`.

3. **Cập nhật**:
   - `CLAUDE.md`: đổi ☐ → ✅ ở dòng pattern đó.
   - `PROGRESS.md`: thêm dòng "Hoàn thành <Pattern> — <ngày>" vào mục "Bài đã hoàn thành" (tạo mục nếu chưa có).
   - `README.md` của bài: nếu mục "Câu hỏi còn thắc mắc" còn câu chưa trả lời, hỏi người học có muốn giải đáp trước không.

4. **Commit** (chỉ khi người học đồng ý):
   - Kiểm tra `git status`, không add file build (`*.exe`, `*.o`, `demo`) hay file trong `00_Document/`.
   - Message: `Add <PatternName> pattern example`.
   - Không push trừ khi người học yêu cầu.

5. **Giới thiệu bài tiếp theo** trong một câu (pattern kế tiếp trong bảng và nó giải quyết vấn đề gì còn tồn tại ở bài vừa học), hỏi người học muốn bắt đầu luôn không.
