# CLAUDE.md

Repo học tập cá nhân: **Embedded C Programming Design Patterns** (theo sách của Martin Schröder, tài liệu trong `00_Document/`).
Mục tiêu: hiểu *tại sao* mỗi pattern tồn tại, viết lại được bằng tay, và biết khi nào dùng / không dùng trong firmware thật.

## Cách Claude hỗ trợ

- **Giải thích bằng tiếng Việt**, giữ nguyên thuật ngữ kỹ thuật tiếng Anh (opaque pointer, callback, vtable, mutex, ...).
- **Ưu tiên dạy hơn làm hộ.** Khi tôi học một pattern mới:
  1. Nêu vấn đề mà pattern giải quyết (code "trước khi có pattern" trông thế nào, đau ở đâu).
  2. Giải thích ý tưởng cốt lõi + sơ đồ **PlantUML** (xem mục "Sơ đồ" bên dưới).
  3. Đưa ví dụ code tối giản, sát với embedded (driver UART/GPIO/sensor, không phải ví dụ "Animal/Dog").
  4. Chỉ ra trade-off: RAM/Flash, tốc độ, độ phức tạp, testability.
  5. Gợi ý bài tập nhỏ để tôi tự làm.
- Mỗi pattern có một **bài giảng chi tiết lưu thành `NN_<PatternName>/LECTURE.md`**: giải thích đầy đủ mục đích, cách làm, *tại sao* từng quy tắc tồn tại, đọc code mẫu từng phần, ví dụ trên phần cứng thật, lỗi hay gặp, quiz có đáp án (ẩn trong `<details>`) và bài tập. Code trong bài giảng phải compile được.
- Khi tôi đưa code của mình: **review trước, chỉ ra lỗi và lý do**, gợi ý hướng sửa; chỉ viết lại toàn bộ khi tôi yêu cầu.
- Nếu tôi hiểu sai khái niệm thì nói thẳng, đừng xuôi theo.
- Liên hệ với các pattern đã học trước đó (ví dụ: Virtual API xây trên Object + Callback).
- Không bịa nội dung sách. Nếu không chắc sách viết gì, nói rõ đó là giải thích chung, không phải trích từ sách.

## Lộ trình pattern (theo đúng thứ tự trong chương Introduction của sách)

`00_Document/` hiện chỉ có chương **Introduction** — nội dung chi tiết từng pattern chưa có, nên phần giải thích là kiến thức chung (ghi rõ điều đó), trừ khi tôi bổ sung thêm chương.

| # | Nhóm | Pattern | Ý chính (theo Introduction) | Trạng thái |
|---|------|---------|-----------------------------|-----------|
| 01 | Creational | Object | Pattern quan trọng nhất trong C; gỡ dependency, bắt buộc refactor hợp lý | ☐ |
| 02 | Creational | Opaque | Ẩn implementation, cấm truy cập member ngoài file .c; xử lý cấp phát bộ nhớ | ☐ |
| 03 | Creational | Singleton | Ép chỉ có một instance, thể hiện rõ ý đồ | ☐ |
| 04 | Creational | Factory | Tạo cấu trúc dữ liệu phức tạp từ dữ liệu cấu hình | ☐ |
| 05 | Structural | Callback | Tránh rối dependency; không dùng raw function pointer tùy tiện | ☐ |
| 06 | Structural | Inheritance | Quy tắc thể hiện quan hệ kế thừa giữa object trong C | ☐ |
| 07 | Structural | Virtual API | Nhiều implementation chung một API, type-safe, không dùng `void *` | ☐ |
| 08 | Structural | Bridge | Nối hai hierarchy độc lập, dựa trên Virtual API | ☐ |
| 09 | Behavioral | Return Value | Quy ước giá trị trả về thống nhất, luôn kiểm tra | ☐ |
| 10 | Concurrency | Concurrency | Pattern tổng quát; concurrency để giải quyết responsiveness | ☐ |
| 11 | Concurrency | Spinlock | Đồng bộ app ↔ ISR và giữa nhiều CPU | ☐ |
| 12 | Concurrency | Semaphore | Báo hiệu từ ISR sang thread; spinlock + thread queue | ☐ |
| 13 | Concurrency | Mutex | Mutual exclusion giữa các thread (trên mức interrupt) | ☐ |
| 14 | Concurrency | Conditional | Nhiều thread chờ một event, được đánh thức cùng lúc | ☐ |

Khi tôi hoàn thành một pattern, cập nhật cột trạng thái (☐ → ✅).

## Cấu trúc mỗi bài học (theo "How to study" của sách)

Mỗi pattern học theo các mục: **Overview → Use Cases → Benefits → Drawbacks → Implementation → Best Practices → Common Pitfalls → Alternatives → Quiz**. `README.md` của mỗi pattern ghi chú theo đúng các mục này, cuối bài có quiz để tôi tự kiểm tra.

Lời khuyên của sách: viết cho chạy trước, rồi refactor theo pattern trước khi coi là xong. Pattern có giá trị khi áp dụng **nhất quán ở mọi nơi**, không phải dùng một lần.

## Skill học tập (`.claude/skills/`)

| Lệnh | Dùng khi |
|------|----------|
| `/lesson [pattern]` | Bắt đầu bài mới: tạo thư mục, code mẫu, `LECTURE.md`, `README.md` |
| `/review-exercise [file]` | Review bài tập: build nghiêm ngặt, soi lỗi pattern + lỗi embedded, không làm hộ |
| `/quiz [pattern\|all]` | Ôn tập từng câu, chấm điểm, ghi vào `PROGRESS.md` |
| `/deep-dive <chủ đề>` | Giải thích sâu khái niệm nền tảng (volatile, ISR, padding, ...) kèm demo trong `99_DeepDive/` |
| `/finish-lesson [pattern]` | Kiểm tra điều kiện hoàn thành, đánh dấu ✅, cập nhật `PROGRESS.md`, commit |

## Cấu trúc thư mục

```
00_Document/                 # Sách, tài liệu tham khảo (không sửa)
99_DeepDive/<topic>/         # Giải thích sâu khái niệm nền tảng + demo (từ /deep-dive)
PROGRESS.md                  # Lịch sử quiz, điểm yếu cần ôn, bài đã hoàn thành
NN_<PatternName>/            # Mỗi pattern một thư mục, đánh số theo thứ tự học
  ├── LECTURE.md             # Bài giảng chi tiết: mục đích, cách làm, lý do từng quy tắc, ví dụ, quiz có đáp án
  ├── README.md              # Ghi chú tóm tắt: vấn đề, ý tưởng, trade-off, câu hỏi còn thắc mắc
  ├── inc/                   # Header public (*.h)
  ├── src/                   # Implementation (*.c)
  ├── main.c                 # Demo chạy trên PC
  └── Makefile
```

Ví dụ: `01_ObjectPattern/`, `02_OpaquePattern/`, `03_SingletonPattern/`, ...

## Quy ước code C

- Chuẩn **C99** (hoặc C11 nếu cần `_Atomic`/`_Static_assert`). Build với cờ:
  `gcc -std=c99 -Wall -Wextra -Werror -pedantic`
- Tư duy embedded dù chạy demo trên PC:
  - Tránh `malloc/free` trừ khi pattern yêu cầu (vd. Factory) — ưu tiên cấp phát tĩnh / do caller cấp bộ nhớ.
  - Dùng kiểu có kích thước cố định từ `<stdint.h>` (`uint8_t`, `int32_t`, ...) và `<stdbool.h>`.
  - Không dùng biến global tùy tiện; state nằm trong struct object.
- Đặt tên theo kiểu module prefix: `uart_init()`, `uart_write()`, struct `struct uart`, hằng `UART_MAX_LEN`.
- API kiểu object: tham số đầu tiên luôn là con trỏ tới object `self`:
  `int uart_write(struct uart *self, const uint8_t *data, size_t len);`
- Hàm trả về mã lỗi `int` (0 = OK, âm = lỗi, theo kiểu `-EINVAL`) — thống nhất với Return Value pattern.
- Mỗi header có include guard, chỉ expose những gì caller cần.
- Comment giải thích **tại sao**, không lặp lại code làm gì.

## Sơ đồ: luôn dùng PlantUML

- Mọi sơ đồ viết bằng khối ` ```plantuml ` trong markdown, không dùng ASCII art hay mermaid.
- Chọn loại sơ đồ theo nội dung: **class** cho struct/quan hệ object, **object** cho bố cục bộ nhớ và instance, **sequence** cho luồng gọi hàm/ISR, **state** cho vòng đời object và state machine, **component** cho quan hệ giữa các module.
- Mở đầu mỗi sơ đồ bằng `set separator none` (nếu không, tên có dấu chấm như `main.c`, `.bss` bị tách thành package lồng nhau) và `skinparam defaultFontName Segoe UI` (hiển thị tiếng Việt đẹp).
- Dòng chữ trong sơ đồ không được bắt đầu bằng `=` (thành tiêu đề) hay `*` (thành gạch đầu dòng).
- Trước khi lưu, render thử để bắt lỗi cú pháp:
  `java -jar ~/tools/plantuml/plantuml.jar -charset UTF-8 -tsvg -failfast2 <file>.puml` (render trong scratchpad, không để file ảnh trong repo).
- Xem sơ đồ: mở file `.md` bằng **Markdown Preview Enhanced** (`Ctrl+K V`). Extension đã được trỏ tới `C:\Users\hoang\tools\plantuml\plantuml.jar`, dùng Java 21 + Graphviz (trong `C:\msys64\ucrt64\bin`).

## Xuất HTML

- `bash tools/md2html.sh` sinh `LECTURE.html` và `README.html` cho mọi bài (hoặc `bash tools/md2html.sh 02_*` cho một bài). Chạy lại mỗi khi sửa file `.md`.
- Dùng pandoc (`%LOCALAPPDATA%\Pandoc\pandoc.exe`) + bộ lọc `tools/md2html.lua` (render PlantUML thành SVG nhúng, đổi link `.md` → `.html`) + `tools/lecture.css` (sáng/tối). File HTML tự chứa, mở thẳng bằng trình duyệt.
- Danh sách đánh số bắt đầu khác 1 (ví dụ `7.`) phải có dòng trống phía trước, nếu không markdown gộp nó vào đoạn văn bên trên.

## Build & chạy

Toolchain: MSYS2 UCRT64 tại `C:\msys64` (gcc 16.2, GNU Make 4.4). `C:\msys64\ucrt64\bin` và `C:\msys64\usr\bin` đã nằm trong User PATH.

```sh
cd NN_<PatternName>
make          # build
make run      # build + chạy demo
make clean
```

Nếu chưa có `make`, build trực tiếp: `gcc -std=c99 -Wall -Wextra -Iinc src/*.c main.c -o demo`.

## Git

- Mỗi pattern hoàn thành = 1 commit, message dạng: `Add <PatternName> pattern example`.
- Không commit file build (`*.o`, `*.exe`, `build/`) và file cài đặt trong `00_Document/`.
