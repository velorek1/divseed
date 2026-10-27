#ifndef DIVSEED_EDITOR_H
#define DIVSEED_EDITOR_H
#include <stddef.h>
#include "utf8.h"
/* Adapted tprg: contiguous document + cached current line, as in DIV.
   Offsets replace movable pointers; capacities and line length are dynamic. */
typedef struct EditSnapshot EditSnapshot;
typedef struct { EditSnapshot *items[64]; size_t count,bytes; } EditHistory;
typedef struct {
 char *buffer,*l,*path,*saved;
 int bom;
 int eol; /* 0 LF, 1 CRLF, 2 CR */
 /* Byte capacities, lengths and line start. */
 size_t capacity,line_capacity,file_lon,lptr,line_size;
 /* One-based rows and scalar columns; anchor remains a byte offset. */
 size_t linea,columna,num_lineas,primera_linea,primera_columna,goal;
 size_t anchor;
 EditHistory undo,redo;
 size_t revision;
 int modified,scroll_free;
 const char *message;
} Document;
typedef enum {ED_LEFT,ED_RIGHT,ED_UP,ED_DOWN,ED_HOME,ED_END,ED_BACKSPACE,ED_DELETE,ED_ENTER} EditKey;
Document *document_new(void);
void document_free(Document *d);
int document_text(Document *d,const char *text);
int document_key(Document *d,EditKey key);
void document_page(Document *d,int direction,size_t rows,int extend);
int document_find(Document *d,const char *query);
int document_replace(Document *d,const char *query,const char *replacement,int all);
void document_view(Document *d,size_t cols,size_t rows);
size_t document_position(const Document *d);
int document_selection(const Document *d,size_t *start,size_t *end);
void document_select_all(Document *d);
void document_move(Document *d,EditKey key,int extend);
void document_point(Document *d,size_t row,size_t column,int extend);
int document_paste(Document *d,const char *text);
char *document_copy(const Document *d);
int document_delete_selection(Document *d);
int document_undo(Document *d);
int document_redo(Document *d);
Document *document_open(const char *path, const char **error);
int document_save(Document *d,const char *path,int overwrite);
void document_clear_history(Document *d);
void document_mark_saved(Document *d,char *saved);
#endif
