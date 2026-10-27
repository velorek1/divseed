#ifndef DIVSEED_SYNTAX_H
#define DIVSEED_SYNTAX_H
#include <stddef.h>
enum {C_TEXT=4,C_KEYWORD=48,C_COMMENT,C_STRING,C_NUMBER,C_PREPROCESSOR};
/* Byte-indexed colors: does not change source bytes or editor positions. */
void c_highlight(const char *text,size_t length,unsigned char *colors);
int c_filename(const char *path);
#endif
