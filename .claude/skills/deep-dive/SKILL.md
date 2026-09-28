---
name: deep-dive
description: Giải thích sâu một khái niệm embedded C nền tảng (volatile, ISR, memory layout, alignment/padding, linker script, endianness, bit manipulation, stack vs static, const/Flash...) kèm demo chạy được trên PC. Dùng khi người học hỏi "X là gì", "tại sao phải Y", "giải thích kỹ Z", "/deep-dive <chủ đề>".
---

# Deep dive một khái niệm embedded

Tham số: chủ đề (vd. `volatile`, `struct padding`, `ISR`, `endianness`).

## Nội dung giải thích (tiếng Việt, thuật ngữ giữ tiếng Anh)

1. **Một câu định nghĩa** dễ hiểu.
2. **Vấn đề thực tế**: bug gì xảy ra nếu không hiểu khái niệm này — kể một tình huống firmware cụ thể.
3. **Cơ chế bên dưới**: compiler/CPU/bộ nhớ làm gì. Vẽ sơ đồ bằng **PlantUML** theo mục "Sơ đồ" trong `CLAUDE.md` (object diagram cho bố cục bộ nhớ, sequence/timing diagram cho timeline ISR, ...) và render thử trước khi lưu.
4. **Demo chạy trên PC**:
   - Viết vào `99_DeepDive/<topic>/main.c` (+ `Makefile` copy từ `01_ObjectPattern/`).
   - Build và chạy thật; đưa output thật vào giải thích.
   - Khi cần xem compiler sinh gì: `gcc -O2 -S` hoặc `objdump -d`, trích đoạn assembly ngắn và giải thích từng dòng.
   - Nếu hành vi chỉ xảy ra trên phần cứng thật (vd. thanh ghi thay đổi ngoài chương trình, ISR thật) → **nói rõ demo PC chỉ mô phỏng**, phần nào không tái hiện được và vì sao.
5. **Liên hệ với design pattern** trong repo (vd. `volatile` ↔ Spinlock bài 11, padding ↔ Object Pattern).
6. **Lỗi hay gặp** và **quy tắc ngón tay cái**.
7. **2–3 câu tự kiểm tra** có đáp án trong `<details>`.

## Lưu lại

Ghi toàn bộ giải thích vào `99_DeepDive/<topic>/README.md` để người học đọc lại. Trong câu trả lời chat chỉ tóm tắt ngắn và link tới file.

Không bịa số liệu phần cứng (địa chỉ thanh ghi, số chu kỳ lệnh). Nếu nêu ví dụ chip cụ thể mà không chắc, nói rõ là ví dụ minh họa.
