/* Adapted from DIV Games Studio src/divedit.c, commit
 * 77556912335d0a1b0efea855d4eb2490cf382128. GPL-3.0.
 * See NOTICE: read_line/write_line, f_insertar, f_suprimir,
 * f_backspace, f_inicio/f_fin and line navigation retain DIV's line cache
 * design. Explicit Document replaces v.prg; no trimming, fixed line limit,
 * compiler state, selection globals, virtual columns or implicit CRLF. */
#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static void history_clear(EditHistory *h);
static int reserve(char **p,size_t *cap,size_t need) {
 if(need<=*cap) return 1;
 size_t n=*cap?*cap:64;
 while(n<need) { if(n>SIZE_MAX/2) {n=need;break;} n*=2; }
 char *next=realloc(*p,n); if(!next) return 0;
 *p=next; *cap=n; return 1;
}
/* Reserve before mutation. Allocation failure preserves text and cursor. */
static int prepare(Document *d,size_t extra) {
 if(extra>SIZE_MAX-d->file_lon-1 ||
    !reserve(&d->buffer,&d->capacity,d->file_lon+extra+1) ||
    !reserve(&d->l,&d->line_capacity,d->file_lon+extra+1)) {
  d->message="Out of memory: text unchanged"; return 0;
 }
 return 1;
}
static void read_line(Document *d) {
 size_t p=d->lptr,len=0;
 while(p<d->file_lon && d->buffer[p]!='\n') {p++;len++;}
 if(len) memcpy(d->l,d->buffer+d->lptr,len);
 d->l[len]=0; d->line_size=len;
}
static void avanza_lptr(Document *d) {
 size_t p=d->lptr;
 while(p<d->file_lon && d->buffer[p]!='\n') p++;
 if(p<d->file_lon) {d->lptr=p+1;d->linea++;}
}
static void retrocede_lptr(Document *d) {
 if(d->lptr) {
  size_t p=d->lptr-1;
  while(p>0 && d->buffer[p-1]!='\n') p--;
  d->lptr=p;d->linea--;
 }
}
static void f_right(Document *d) {if(d->columna<=utf8_count(d->l,d->line_size)) d->columna++;}
static void f_left(Document *d) {if(d->columna>1) d->columna--;}
static void f_inicio(Document *d) {d->primera_columna=d->columna=1;}
static void f_fin(Document *d) {d->columna=utf8_count(d->l,d->line_size)+1;}
static void f_suprimir(Document *d) {
 size_t p=document_position(d);
 if(p>=d->file_lon) return;
 size_t end=utf8_next(d->buffer,d->file_lon,p);
 if(d->buffer[p]=='\n') d->num_lineas--;
 memmove(d->buffer+p,d->buffer+end,d->file_lon-end+1);
 d->file_lon-=end-p;read_line(d);d->modified=1;
}
static void f_backspace(Document *d) {
 if(d->columna!=1) {f_left(d);f_suprimir(d);}
 else if(d->lptr) {retrocede_lptr(d);read_line(d);f_fin(d);f_suprimir(d);}
}
static void f_enter(Document *d) {
 size_t p=document_position(d);
 memmove(d->buffer+p+1,d->buffer+p,d->file_lon-p+1);
 d->buffer[p]='\n'; d->file_lon++;d->num_lineas++;
 d->lptr=p+1;d->linea++;d->columna=1;read_line(d);d->modified=1;
}
Document *document_new(void) {
 Document *d=calloc(1,sizeof(*d)); if(!d) return NULL;
 if(!prepare(d,0)) {document_free(d);return NULL;}
 d->buffer[0]=d->l[0]=0;
 d->linea=d->columna=d->goal=d->num_lineas=d->primera_linea=d->primera_columna=1;
 return d;
}
void document_free(Document *d) {if(d) {history_clear(&d->undo);history_clear(&d->redo);free(d->path);free(d->saved);free(d->buffer);free(d->l);free(d);}}
static int text_raw(Document *d,const char *text) {
 size_t n=strlen(text),p=document_position(d);
 if(!prepare(d,n)) return 0;
 memmove(d->buffer+p+n,d->buffer+p,d->file_lon-p+1);
 memcpy(d->buffer+p,text,n);d->file_lon+=n;read_line(d);
 d->columna+=utf8_count(text,n);d->goal=d->columna;d->modified=1;d->message=NULL;return 1;
}
static int key_raw(Document *d,EditKey key) {
 if(key==ED_ENTER && !prepare(d,1)) return 0;
 d->message=NULL;
 switch(key) {
 case ED_LEFT:f_left(d);break;
 case ED_RIGHT:f_right(d);break;
 case ED_HOME:f_inicio(d);break;
 case ED_END:f_fin(d);break;
 case ED_BACKSPACE:f_backspace(d);break;
 case ED_DELETE:f_suprimir(d);break;
 case ED_ENTER:f_enter(d);break;
 case ED_UP:retrocede_lptr(d);read_line(d);break;
 case ED_DOWN:avanza_lptr(d);read_line(d);break;
 }
 if(key==ED_UP || key==ED_DOWN) d->columna=d->goal<utf8_count(d->l,d->line_size)+1?d->goal:utf8_count(d->l,d->line_size)+1;
 else d->goal=d->columna;
 return 1;
}
void document_view(Document *d,size_t cols,size_t rows) {
 if(d->scroll_free) return;
 if(d->columna<d->primera_columna) d->primera_columna=d->columna;
 if(d->columna-d->primera_columna>=cols) d->primera_columna=d->columna-cols+1;
 if(d->linea<d->primera_linea) d->primera_linea=d->linea;
 if(d->linea-d->primera_linea>=rows) d->primera_linea=d->linea-rows+1;
}

/* Selection and history adapter. New code: the original editor's selection
 * globals cannot represent independent selections in several documents. */
struct EditSnapshot {
 char *text;
 size_t length,position,anchor,first_row,first_col,goal;
 int modified;
};
size_t document_position(const Document *d) {return d->lptr+utf8_offset(d->l,d->line_size,d->columna-1);}
int document_selection(const Document *d,size_t *start,size_t *end) {
 size_t p=document_position(d);
 *start=p<d->anchor?p:d->anchor;*end=p>d->anchor?p:d->anchor;
 return *start!=*end;
}
static void seek_position(Document *d,size_t p) {
 d->scroll_free=0;
 if(p>d->file_lon) p=d->file_lon;
 d->lptr=0;d->linea=1;d->num_lineas=1;
 for(size_t i=0;i<d->file_lon;i++) if(d->buffer[i]=='\n') {
  d->num_lineas++;if(i<p) {d->lptr=i+1;d->linea++;}
 }
 d->columna=utf8_count(d->buffer+d->lptr,p-d->lptr)+1;read_line(d);d->goal=d->columna;
}
void document_point(Document *d,size_t row,size_t column,int extend) {
 size_t p=0,r=1;
 while(p<d->file_lon && r<row) if(d->buffer[p++]=='\n') r++;
 size_t c=1;
 while(p<d->file_lon && d->buffer[p]!='\n' && c<column) {p=utf8_next(d->buffer,d->file_lon,p);c++;}
 seek_position(d,p);if(!extend) d->anchor=p;d->message=NULL;
}
void document_select_all(Document *d) {seek_position(d,d->file_lon);d->anchor=0;}
static void snapshot_free(EditSnapshot *s) {if(s) {free(s->text);free(s);}}
static EditSnapshot *snapshot(Document *d) {
 EditSnapshot *s=malloc(sizeof(*s));
 if(!s) goto fail;
 s->text=malloc(d->file_lon+1);
 if(!s->text) {free(s);goto fail;}
 memcpy(s->text,d->buffer,d->file_lon+1);
 s->length=d->file_lon;s->position=document_position(d);s->anchor=d->anchor;
 s->first_row=d->primera_linea;s->first_col=d->primera_columna;s->goal=d->goal;s->modified=d->modified;
 return s;
fail:d->message="Out of memory: edit cancelled";return NULL;
}
static void history_clear(EditHistory *h) {
 for(size_t i=0;i<h->count;i++) snapshot_free(h->items[i]);
 h->count=h->bytes=0;
}
static void history_push(EditHistory *h,EditSnapshot *s) {
 /* 64 transactions and a soft 16 MiB budget per stack; a single larger
    snapshot is retained so a successful edit never silently loses undo. */
 while(h->count && (h->count==64 || h->bytes+s->length+1>16*1024*1024)) {
  h->bytes-=h->items[0]->length+1;snapshot_free(h->items[0]);
  memmove(h->items,h->items+1,(--h->count)*sizeof(h->items[0]));
 }
 h->items[h->count++]=s;h->bytes+=s->length+1;
}
static void commit(Document *d,EditSnapshot *old) {
 if(old->length!=d->file_lon || memcmp(old->text,d->buffer,d->file_lon)) {
  d->revision++;history_clear(&d->redo);history_push(&d->undo,old);
 } else snapshot_free(old);
 d->anchor=document_position(d);d->message=NULL;d->scroll_free=0;
 d->modified=d->saved ? strcmp(d->buffer,d->saved)!=0 : d->file_lon!=0;
}
static void erase_selection(Document *d) {
 size_t a,b;if(!document_selection(d,&a,&b)) return;
 memmove(d->buffer+a,d->buffer+b,d->file_lon-b+1);
 d->file_lon-=b-a;seek_position(d,a);d->anchor=a;d->modified=1;
}
int document_text(Document *d,const char *text) {
 if(!utf8_valid(text,strlen(text),0)) {d->message="Invalid UTF-8 or unsupported control character";return 0;}
 if(!*text) return 1;
 if(!prepare(d,strlen(text))) return 0;
 EditSnapshot *old=snapshot(d);if(!old) return 0;
 erase_selection(d);text_raw(d,text);commit(d,old);return 1;
}
void document_move(Document *d,EditKey key,int extend) {
 d->scroll_free=0;
 size_t a,b;
 if(!extend && document_selection(d,&a,&b) && (key==ED_LEFT || key==ED_RIGHT))
  seek_position(d,key==ED_LEFT?a:b);
 else if(extend && key==ED_LEFT && d->columna==1 && d->lptr) seek_position(d,document_position(d)-1);
 else if(extend && key==ED_RIGHT && d->columna==utf8_count(d->l,d->line_size)+1 && document_position(d)<d->file_lon) seek_position(d,document_position(d)+1);
 else key_raw(d,key);
 if(!extend) d->anchor=document_position(d);
}
int document_key(Document *d,EditKey key) {
 if(key!=ED_ENTER && key!=ED_BACKSPACE && key!=ED_DELETE) {document_move(d,key,0);return 1;}
 if(!prepare(d,key==ED_ENTER?1:0)) return 0;
 EditSnapshot *old=snapshot(d);if(!old) return 0;
 size_t a,b;int selected=document_selection(d,&a,&b);
 erase_selection(d);
 if(!selected || key==ED_ENTER) key_raw(d,key);
 commit(d,old);return 1;
}
int document_delete_selection(Document *d) {
 size_t a,b;if(!document_selection(d,&a,&b)) return 1;
 return document_key(d,ED_DELETE);
}
char *document_copy(const Document *d) {
 size_t a,b;if(!document_selection(d,&a,&b)) return NULL;
 char *text=malloc(b-a+1);if(text) {memcpy(text,d->buffer+a,b-a);text[b-a]=0;}return text;
}
int document_paste(Document *d,const char *text) {
 size_t n=strlen(text);if(!n) return 1;
 char *normalized=malloc(n+1);if(!normalized) {d->message="Out of memory: paste cancelled";return 0;}
 size_t used=0;
 for(size_t i=0;i<n;i++) {
  unsigned char c=(unsigned char)text[i];
  if(c=='\r') {if(text[i+1]=='\n') i++;c='\n';}
 
  normalized[used++]=(char)c;
 }
 normalized[used]=0;
 if(!utf8_valid(normalized,used,1)) {free(normalized);d->message="Paste rejected: invalid UTF-8 or control character";return 0;}
 if(!prepare(d,used)) {free(normalized);return 0;}
 EditSnapshot *old=snapshot(d);if(!old) {free(normalized);return 0;}
 erase_selection(d);size_t p=document_position(d);
 memmove(d->buffer+p+used,d->buffer+p,d->file_lon-p+1);
 memcpy(d->buffer+p,normalized,used);d->file_lon+=used;
 free(normalized);seek_position(d,p+used);d->modified=1;commit(d,old);return 1;
}
static int restore_history(Document *d,EditHistory *from,EditHistory *to) {
 if(!from->count) {d->message="No more history";return 0;}
 EditSnapshot *s=from->items[from->count-1];
 if(!prepare(d,s->length>d->file_lon?s->length-d->file_lon:0)) return 0;
 EditSnapshot *current=snapshot(d);if(!current) return 0;
 from->count--;from->bytes-=s->length+1;
 d->revision++;memcpy(d->buffer,s->text,s->length+1);d->file_lon=s->length;
 seek_position(d,s->position);d->anchor=s->anchor;d->goal=s->goal;
 d->primera_linea=s->first_row;d->primera_columna=s->first_col;d->modified=s->modified;
 snapshot_free(s);history_push(to,current);d->message=NULL;
 d->modified=d->saved ? strcmp(d->buffer,d->saved)!=0 : d->file_lon!=0;return 1;
}
int document_undo(Document *d) {return restore_history(d,&d->undo,&d->redo);}
int document_redo(Document *d) {return restore_history(d,&d->redo,&d->undo);}

void document_mark_saved(Document *d,char *saved) {free(d->saved);d->saved=saved;d->modified=0;}

void document_clear_history(Document *d) {history_clear(&d->undo);history_clear(&d->redo);}

void document_page(Document *d,int direction,size_t rows,int extend) {
 size_t goal=d->goal;
 for(size_t i=0;i<rows;i++) document_move(d,direction<0?ED_UP:ED_DOWN,extend);
 d->goal=goal;
 if(direction<0) d->primera_linea=d->primera_linea>rows?d->primera_linea-rows:1;
 else {size_t max=d->num_lineas>rows?d->num_lineas-rows+1:1;
 d->primera_linea=d->primera_linea+rows>max?max:d->primera_linea+rows;}
}
int document_find(Document *d,const char *query) {
 size_t n=strlen(query);if(!n || !utf8_valid(query,n,1)) return 0;
 size_t pos=document_position(d);const char *hit=strstr(d->buffer+pos,query);
 if(!hit) hit=strstr(d->buffer,query);
 if(!hit) {d->message="Text not found";return 0;}
 size_t a=(size_t)(hit-d->buffer);seek_position(d,a+n);d->anchor=a;d->message=NULL;return 1;
}
int document_replace(Document *d,const char *query,const char *replacement,int all) {
 size_t n=strlen(query),r=strlen(replacement),a,b,count=0;
 if(!n || !utf8_valid(query,n,1) || !utf8_valid(replacement,r,1)) return 0;
 if(!all) {
  if(!document_selection(d,&a,&b) || b-a!=n || memcmp(d->buffer+a,query,n)) {
   if(!document_find(d,query)) return 0;
  }
  return r?document_paste(d,replacement):document_delete_selection(d);
 }
 for(const char *hit=d->buffer;(hit=strstr(hit,query));hit+=n) count++;
 if(!count) {d->message="Text not found";return 0;}
 if(r>n && count>(SIZE_MAX-d->file_lon-1)/(r-n)) return 0;
 size_t size=r>=n?d->file_lon+count*(r-n):d->file_lon-count*(n-r);
 char *text=malloc(size+1);if(!text) {d->message="Out of memory: text unchanged";return 0;}
 const char *src=d->buffer,*hit;char *out=text;
 while((hit=strstr(src,query))) {size_t len=(size_t)(hit-src);memcpy(out,src,len);out+=len;memcpy(out,replacement,r);out+=r;src=hit+n;}
 strcpy(out,src);
 if(!prepare(d,size>d->file_lon?size-d->file_lon:0)) {free(text);return 0;}
 EditSnapshot *old=snapshot(d);if(!old) {free(text);return 0;}
 memcpy(d->buffer,text,size+1);d->file_lon=size;free(text);seek_position(d,0);commit(d,old);return 1;
}
