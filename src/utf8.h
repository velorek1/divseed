#ifndef DIVSEED_UTF8_H
#define DIVSEED_UTF8_H
#include <stddef.h>
#include <stdint.h>
/* Byte offsets and Unicode scalar columns are deliberately separate. */
size_t utf8_next(const char *s,size_t n,size_t p);
size_t utf8_prev(const char *s,size_t p);
uint32_t utf8_decode(const char *s,size_t n,size_t *p);
size_t utf8_count(const char *s,size_t n);
size_t utf8_offset(const char *s,size_t n,size_t column);
int utf8_valid(const char *s,size_t n,int multiline);
#endif
