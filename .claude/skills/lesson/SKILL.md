---
name: lesson
description: Bắt đầu bài học design pattern mới trong repo (tạo thư mục NN_<Pattern>, code mẫu, Makefile, LECTURE.md, README.md). Dùng khi người học nói "học bài tiếp", "bắt đầu pattern X", "/lesson <pattern>".
---

# Bắt đầu một bài học pattern mới

Tham số: tên pattern (vd. `Opaque`). Không có tham số → lấy pattern đầu tiên còn ☐ trong bảng lộ trình của `CLAUDE.md`.

## Các bước

1. **Đọc lại bối cảnh**
   - Bảng lộ trình và quy ước trong `CLAUDE.md`.
   - Tìm trong `00_Document/` xem có chương về pattern này chưa (`pdftotext -layout <file> -` để đọc PDF). Có → bám theo sách và trích dẫn. Không có → ghi rõ ở đầu bài: "kiến thức chung, không trích từ sách".
   - Đọc `LECTURE.md` của các bài trước để liên hệ (vd. Opaque vá lỗ hổng "field public" của Object).
   - Đọc `PROGRESS.md` (nếu có) để biết điểm yếu của người học và nhấn mạnh lại.

2. **Tạo thư mục** `NN_<PatternName>/` (NN = số thứ tự trong bảng) gồm `inc/`, `src/`, `main.c`, `Makefile` — copy Makefile từ `01_ObjectPattern/` để giữ nhất quán.

3. **Viết code mẫu**
   - Ví dụ embedded thật (UART, SPI, I2C, GPIO, sensor, ring buffer), không dùng Animal/Dog.
   - Tuân thủ mọi quy ước code C trong `CLAUDE.md` (C99, `self` đầu tiên, trả `int`, không `malloc` trừ khi pattern cần).
   - Nếu hợp lý, tái dùng module từ bài trước để thấy các pattern xếp chồng lên nhau.

4. **Build và chạy** bằng `make run` (PATH cần có `/c/msys64/ucrt64/bin` và `/c/msys64/usr/bin`). Phải build sạch với `-Wall -Wextra -Werror -pedantic`. Ghi lại output thật để đưa vào bài giảng — không bịa output.

5. **Viết `LECTURE.md`** theo khung của `01_ObjectPattern/LECTURE.md`:
   bối cảnh → vấn đề (code "trước") → ý tưởng cốt lõi + sơ đồ → các quy tắc và *tại sao* → đọc code mẫu từng phần → trên phần cứng thật → test → trade-off → khi nào không dùng → lỗi hay gặp → quy trình refactor → checklist → quiz có đáp án trong `<details>` → bài tập (cơ bản / refactor / nâng cao).
   Mọi đoạn code trong bài giảng phải compile được — viết ra scratchpad và build thử trước.
   Mọi sơ đồ vẽ bằng **PlantUML** theo mục "Sơ đồ" trong `CLAUDE.md`; tối thiểu có: sơ đồ "trước/sau" của vấn đề, class diagram cho các struct, và sequence diagram nếu có luồng gọi đáng chú ý. Render thử từng sơ đồ trước khi lưu.

6. **Viết `README.md`** tóm tắt theo các mục của sách: Overview → Use Cases → Benefits → Drawbacks → Implementation → Best Practices → Common Pitfalls → Alternatives → Quiz, có link sang `LECTURE.md` và mục "Câu hỏi còn thắc mắc" để trống.

7. **Trả lời người học** bằng tiếng Việt: tóm tắt ý chính của bài trong vài đoạn, link tới `LECTURE.md`, giao bài tập. Không đánh dấu ✅ — việc đó để skill `finish-lesson`.
