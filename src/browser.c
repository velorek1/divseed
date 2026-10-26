#define _XOPEN_SOURCE 700
#include "browser.h"
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
void browser_free(FileBrowser *b) {
 for(size_t i=0;i<b->ndirs;i++) free(b->dirs[i].name);
 for(size_t i=0;i<b->nfiles;i++) free(b->files[i].name);
 free(b->dirs);free(b->files);b->dirs=b->files=NULL;b->ndirs=b->nfiles=0;
}
int browser_join(const FileBrowser *b,const char *name,char *out,size_t capacity) {
 int n=name[0]=='/'?snprintf(out,capacity,"%s",name):snprintf(out,capacity,"%s/%s",b->path,name);
 return n>=0 && (size_t)n<capacity;
}
static int compare(const void *a,const void *b) {return strcmp(((const BrowserEntry*)a)->name,((const BrowserEntry*)b)->name);}
static int append(BrowserEntry **entries,size_t *count,const char *name,int directory) {
 char *copy=strdup(name);if(!copy) return 0;
 BrowserEntry *p=realloc(*entries,(*count+1)*sizeof(*p));if(!p) {free(copy);return 0;}
 *entries=p;p[*count]=(BrowserEntry){copy,directory};(*count)++;return 1;
}
int browser_read(FileBrowser *b,const char *path) {
 char *resolved=realpath(path,NULL);if(!resolved) {b->error="Cannot access directory";return 0;}
 if(strlen(resolved)>=sizeof b->path) {free(resolved);b->error="Directory path too long";return 0;}
 DIR *dir=opendir(resolved);if(!dir) {free(resolved);b->error="Cannot read directory";return 0;}
 FileBrowser next={0};next.text_only=b->text_only;strcpy(next.path,resolved);free(resolved);
 struct dirent *entry;int ok=1;
 while((errno=0,entry=readdir(dir))) {
  if(!strcmp(entry->d_name,".")) continue;
  char full[4096];struct stat st;
  if(!browser_join(&next,entry->d_name,full,sizeof full) || stat(full,&st)) continue;
  int directory=S_ISDIR(st.st_mode);
  if(!directory && !S_ISREG(st.st_mode)) continue;
  if(!directory && next.text_only) {const char *dot=strrchr(entry->d_name,'.');if(!dot || strcmp(dot,".txt")) continue;}
  if(next.ndirs+next.nfiles>=10000) {next.error="Showing first 10000 entries; use a typed path for others";break;}
  if(!append(directory?&next.dirs:&next.files,directory?&next.ndirs:&next.nfiles,entry->d_name,directory)) {ok=0;break;}
 }
 if(errno) ok=0;
 closedir(dir);
 if(!ok) {browser_free(&next);b->error="Directory listing failed";return 0;}
 if(next.ndirs>1) qsort(next.dirs,next.ndirs,sizeof(*next.dirs),compare);
 if(next.nfiles>1) qsort(next.files,next.nfiles,sizeof(*next.files),compare);
 browser_free(b);*b=next;return 1;
}
