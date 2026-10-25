#ifndef ERRSTR_H
#define ERRSTR_H

/*
 * Đổi mã trả về (0 hoặc -Exxx) thành tên ngắn như "EIO" để in log.
 * Giá trị số của errno khác nhau giữa các nền tảng, nên log phải in tên,
 * không in số.
 */
const char *errstr(int ret);

#endif /* ERRSTR_H */
