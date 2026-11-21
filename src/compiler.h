#ifndef DIVSEED_COMPILER_H
#define DIVSEED_COMPILER_H

#define COMPILER_COLUMNS 75
int compiler_available(void);
int compiler_busy(void);
const char *compiler_start(const char *path,int run);
void compiler_poll(void);
const char *compiler_status(void);
int compiler_line_count(void);
const char *compiler_line(int row);
void compiler_destroy(void);

#endif
