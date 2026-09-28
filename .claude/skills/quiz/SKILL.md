---
name: quiz
description: Kiểm tra kiến thức người học về các pattern/khái niệm embedded đã học bằng câu hỏi từng câu một, chấm điểm và ghi lại vào PROGRESS.md. Dùng khi người học nói "quiz", "kiểm tra tôi", "ôn bài", "/quiz [pattern|all]".
---

# Quiz ôn tập

Tham số: tên pattern, `all` (mọi bài đã học), hoặc trống (ưu tiên điểm yếu trong `PROGRESS.md`, rồi tới bài mới học gần nhất).

## Cách ra đề

- Chỉ hỏi về bài **đã có thư mục** trong repo (đã học). Đọc `LECTURE.md` của bài đó để lấy nội dung.
- **5 câu**, trộn các dạng:
  - **Tại sao**: "Tại sao `init` nhận `self` thay vì trả về struct?"
  - **Tìm lỗi**: đưa đoạn code 5–15 dòng có 1 lỗi, hỏi lỗi ở đâu và hậu quả.
  - **Tình huống thiết kế**: "Board có 3 cảm biến trên 2 bus I2C, bạn tổ chức object thế nào?"
  - **Dự đoán output**: đoạn code nhỏ, hỏi in ra gì (phải chạy thử code trước để chắc đáp án đúng).
  - **So sánh**: "Khi nào chọn Singleton thay vì Object thường?"
- Không hỏi câu thuộc lòng định nghĩa. Không lặp lại nguyên văn câu quiz trong `LECTURE.md`.
- Ưu tiên chủ đề trong mục "Điểm yếu cần ôn" của `PROGRESS.md`.

## Cách hỏi

- Hỏi **từng câu một**, chờ người học trả lời rồi mới sang câu tiếp.
- Chấm mỗi câu: ✅ đúng / 🟡 đúng một phần / ❌ sai. Giải thích ngắn gọn phần thiếu hoặc sai — nói thẳng nếu hiểu sai khái niệm.
- Người học nói "không biết" → giải thích luôn, tính ❌, không trách.

## Kết thúc

1. Tổng kết: điểm x/5, chủ đề nắm chắc, chủ đề cần ôn, gợi ý đọc lại mục nào trong `LECTURE.md`.
2. Cập nhật `PROGRESS.md` ở gốc repo (tạo nếu chưa có) theo khung:

   ```markdown
   # Tiến độ học

   ## Lịch sử quiz
   | Ngày | Chủ đề | Điểm | Ghi chú |
   |------|--------|------|---------|

   ## Điểm yếu cần ôn
   - [ ] <khái niệm> — <hiểu sai ở đâu> (phát hiện ngày ...)
   ```

   Thêm một dòng vào "Lịch sử quiz". Thêm điểm yếu mới; đánh dấu `[x]` điểm yếu cũ nếu lần này đã trả lời đúng.
