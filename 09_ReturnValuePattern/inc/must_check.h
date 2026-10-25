#ifndef MUST_CHECK_H
#define MUST_CHECK_H

/*
 * Đánh dấu hàm trả mã lỗi. Caller gọi mà bỏ qua giá trị trả về thì gcc/clang
 * cảnh báo (-Wunused-result), và -Werror biến cảnh báo đó thành lỗi build.
 * Quy tắc "luôn kiểm tra giá trị trả về" nhờ vậy được compiler ép, không chỉ
 * dựa vào kỷ luật của người viết code.
 */
#if defined(__GNUC__) || defined(__clang__)
#define MUST_CHECK __attribute__((warn_unused_result))
#else
#define MUST_CHECK
#endif

#endif /* MUST_CHECK_H */
