/* New POSIX file adapter; no compiler or DIV-specific file transformations. */
#define _POSIX_C_SOURCE 200809L
#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
Document *document_open(const char *path,const char **error) {
 *error="Cannot open file";
 FILE *f=fopen(path,"rb");if(!f) return NULL;
 struct stat st;
 if(fstat(fileno(f),&st) || !S_ISREG(st.st_mode) || st.st_size>16*1024*1024) {
  *error="Only regular text files up to 16 MiB are supported";fclose(f);return NULL;
 }
 size_t n=(size_t)st.st_size;char *raw=malloc(n+1);
 if(!raw) {fclose(f);return NULL;}
 size_t got=fread(raw,1,n,f);int extra=fgetc(f),bad=ferror(f);fclose(f);
 if(got!=n || extra!=EOF || bad) {free(raw);*error="Read failed or file changed during read";return NULL;}
 if(!utf8_valid(raw,n,1)) {free(raw);*error="Invalid UTF-8 or unsupported control bytes";return NULL;}
 int bom=n>=3 && !memcmp(raw,"\xef\xbb\xbf",3);
 size_t out=0;int kind=-1;
 for(size_t i=bom?3:0;i<n;i++) {
  unsigned char c=(unsigned char)raw[i];
  if(c=='\r' || c=='\n') {
   int k=c=='\n'?0:2;
   if(c=='\r' && i+1<n && raw[i+1]=='\n') {i++;k=1;}
   if(kind>=0 && kind!=k) {free(raw);*error="Mixed line endings: file not opened or changed";return NULL;}
   kind=k;c='\n';
  }
  raw[out++]=(char)c;
 }
 raw[out]=0;Document *d=document_new();
 if(!d || !document_paste(d,raw)) {document_free(d);free(raw);return NULL;}
 d->path=strdup(path);if(!d->path) {document_free(d);free(raw);return NULL;}
 document_clear_history(d);d->bom=bom;d->eol=kind<0?0:kind;document_mark_saved(d,raw);
 document_point(d,1,1,0);d->primera_linea=d->primera_columna=1;
 /* Loading is not an undoable edit. */
 return d;
}
int document_save(Document *d,const char *path,int overwrite) {
 struct stat st;int exists=lstat(path,&st)==0;
 d->message="Save failed: original file preserved";
 if(!exists && errno!=ENOENT) return 0;
 if(exists && (!S_ISREG(st.st_mode) || st.st_nlink!=1)) {d->message="Refusing to replace a link or non-regular file";return 0;}
 if(exists && !overwrite) {d->message="File exists: confirm overwrite";return 2;}
 if(exists && access(path,W_OK)) {d->message="File is not writable";return 0;}
 if(d->path && !strcmp(path,d->path)) {
  const char *error;Document *disk=document_open(path,&error);
  int same=disk && disk->bom==d->bom && d->saved && !strcmp(disk->buffer,d->saved) && (!strchr(d->saved,'\n') || disk->eol==d->eol);
  document_free(disk);
  if(!same) {d->message="File changed on disk: use Save As to preserve both versions";return 0;}
 }
 char *newpath=strdup(path),*saved=strdup(d->buffer);
 size_t n=strlen(path);char *tmp=malloc(n+16);
 if(!newpath || !saved || !tmp) {free(newpath);free(saved);free(tmp);return 0;}
 snprintf(tmp,n+16,"%s.tmp.XXXXXX",path);int fd=mkstemp(tmp),ok=fd>=0;
 if(ok && exists && fchmod(fd,st.st_mode&0777)) ok=0;
 FILE *f=ok?fdopen(fd,"wb"):NULL;
 if(!f) {if(fd>=0) close(fd);ok=0;}
 else {
  if(d->bom && fwrite("\xef\xbb\xbf",1,3,f)!=3) ok=0;
  for(size_t i=0;i<d->file_lon && ok;i++) {
   int c=(unsigned char)d->buffer[i];
   if(c=='\n' && d->eol) {if(fputc('\r',f)==EOF) ok=0;if(d->eol==2) continue;}
   if(fputc(c,f)==EOF) ok=0;
  }
  if(fflush(f) || fsync(fileno(f))) ok=0;
  if(fclose(f)) ok=0;
 }
 if(ok) {
  /* Recheck the destination before replacing it. */
  struct stat now;int current=lstat(path,&now)==0;
  if(current!=exists || (exists && (now.st_dev!=st.st_dev || now.st_ino!=st.st_ino || now.st_size!=st.st_size || now.st_mtim.tv_sec!=st.st_mtim.tv_sec || now.st_mtim.tv_nsec!=st.st_mtim.tv_nsec))) ok=0;
 }
 if(ok) {
  if(exists) ok=rename(tmp,path)==0;
  else {ok=link(tmp,path)==0;if(ok) unlink(tmp);} /* no accidental clobber */
 }
 if(!ok) {unlink(tmp);free(newpath);free(saved);free(tmp);return 0;}
 free(tmp);free(d->path);d->path=newpath;document_mark_saved(d,saved);d->message="Saved";
 char *parent=strdup(d->path);
 if(parent) {char *slash=strrchr(parent,'/');if(slash) {if(slash==parent) slash[1]=0;else *slash=0;}else strcpy(parent,".");
  int dirfd=open(parent,O_RDONLY|O_DIRECTORY);
  if(dirfd<0 || fsync(dirfd)) d->message="Saved; directory sync failed (durability not confirmed)";
  if(dirfd>=0) close(dirfd);
  free(parent);
 }
 return 1;
}
