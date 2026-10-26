#ifndef DIVSEED_BROWSER_H
#define DIVSEED_BROWSER_H
#include <stddef.h>
typedef struct {char *name;int directory;} BrowserEntry;
typedef struct {
 char path[4096];BrowserEntry *dirs,*files;size_t ndirs,nfiles;
 size_t dir_first,file_first;int text_only;const char *error;
} FileBrowser;
void browser_free(FileBrowser *b);
int browser_read(FileBrowser *b,const char *path);
int browser_join(const FileBrowser *b,const char *name,char *out,size_t capacity);
#endif
