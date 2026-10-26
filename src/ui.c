/* DIV window adapter. See NOTICE for retained drawing and new events. */
#include "ui.h"
#include "div_icons.h"
#include "unicode_font.h"
#include "browser.h"
#include <sys/stat.h>
#define W UI_WIDTH
#define H UI_HEIGHT
#define TX 8
#define TY 38
#define FW 8
#define FH 11
static int animations;
static Uint32 modal_opened;
void ui_animations(int enabled) {animations=enabled;}
static Uint32 opened(void) {return SDL_GetTicks()+1;}
/* DIV div.c:extrude interpolates a white outline before revealing the window.
   Advance on elapsed time here so SDL input never waits in an animation loop. */
static int opening(byte *dst,SDL_Rect r,Uint32 start) {
 if(!animations || !start) return 0;
 Uint32 elapsed=SDL_GetTicks()-(start-1);
 if(elapsed>=180) return 0;
 int step=1+(int)(elapsed/20),w=r.w*step/10,h=r.h*step/10;
 wrectangulo(dst,W,H,10,r.x+(r.w-w)/2,r.y+(r.h-h)/2,w,h);
 return 1;
}
static int zclock,resizing,scroll_drag,scroll_offset;
static int serial,dragging,selecting,dx,dy,pending,quit_ready,suppress_text;
static int dialog,after_save,overwrite;
static int modal_dx,modal_dy,modal_drag,modal_kind,help_first,help_drag;
static SDL_Rect press_rect;
static int press_active;
static byte modal_pixels[W*H];
static const char *help_lines[]={
 "DIVseed - commands and options", "", "DOCUMENTS", "Ctrl+N           New document", "Ctrl+O           Open file", "Ctrl+S           Save", "Ctrl+Shift+S     Save As", "Ctrl+W           Close document", "Esc              Exit (asks about changes)", "", "EDITING", "Arrows           Move cursor", "Home / End       Start / end of line", "Enter            Split line", "Backspace        Delete previous character", "Delete           Delete next character", "Tab              Insert a literal tab", "", "SELECTION AND CLIPBOARD", "Shift + movement Extend selection", "Mouse drag       Select text", "Ctrl+A           Select all", "Ctrl+C / X / V   Copy / cut / paste", "Ctrl+Z           Undo", "Ctrl+Y           Redo", "Ctrl+Shift+Z     Redo", "", "DESKTOP", "Drag title       Move window or dialog", "Bottom-right     Resize editor window", "Title buttons    Minimize / maximize / close", "Wheel / bars     Scroll editor", "Floating menus   Drag their title to move", "F1               Open this help", "", "FILE PICKER", "Click folder     Enter directory", "Double-click     Open selected file", "Enter / Accept   Confirm selection", "Esc / Cancel     Keep editing", "Ctrl+A in field  Clear file name", "Wheel / arrows   Scroll each file list", "Type             All files or *.txt", "", "TEXT FORMAT", "UTF-8            One cell per Unicode scalar", "Missing glyph    Marker; original bytes retained", "LF / CRLF / CR   Uniform endings preserved", "Mixed endings    Rejected without changing file", "", "HELP NAVIGATION", "Wheel, Up/Down, Page Up/Down", "Home / End       First / last help page", "Drag scrollbar   Move through help", "OK, Enter, Esc   Close help"};
#define HELP_ROWS 12
#define HELP_COUNT ((int)(sizeof help_lines/sizeof help_lines[0]))
static char filename[4096],notice[160];
static FileBrowser browser;
static int selected_file=-1;
static int menu_drag=-1,mouse_x=-1,mouse_y=-1;
typedef struct {int x,y,w,h,open,z;const char *title;const char **items;int count;Uint32 opened_at;} Menu;
static const char *main_items[]={"File","Edit","View","Help","Exit MAIN MENU"};
static const char *file_items[]={"New       Ctrl+N","Open      Ctrl+O","Save      Ctrl+S","Save As   Ctrl+Shift+S","Close     Ctrl+W","Exit MAIN MENU"};
static const char *edit_items[]={"Undo","Redo","Cut","Copy","Paste","Select All"};
static const char *view_items[]={"Minimize","Maximize / Restore"};
static const char *help_items[]={"Commands / F1","About"};
static Menu menus[]={ {8,42,190,138,1,0,"MAIN MENU",main_items,5,0}, {170,42,230,160,0,0,"File",file_items,6,0}, {170,208,180,160,0,0,"Edit",edit_items,6,0}, {360,42,220,72,0,0,"View",view_items,2,0}, {360,120,180,72,0,0,"Help",help_items,2,0} };
static byte editor_font[256*FW*FH];
static size_t cols(void) {return (size_t)(v.an-32)/FW;}
static size_t rows(void) {return (size_t)(v.al-64)/FH;}
/* Bounds-safe adaptation of original clipped bitmap drawing. */
void wtexc(byte *dst,int stride,int width,int height,byte *src,
           int x,int y,byte sw,int sh,byte color) {
 for(int j=0;j<sh;j++) for(int i=0;i<sw;i++)
  if(x+i>=0 && x+i<width && y+j>=0 && y+j<height && src[j*sw+i])
   dst[(y+j)*stride+x+i]=color;
}
static const byte *unicode_bitmap(uint32_t cp) {
 if(cp<128) return editor_font+cp*FW*FH;
 size_t low=0,high=sizeof unicode_glyphs/sizeof unicode_glyphs[0];
 while(low<high) {size_t mid=low+(high-low)/2;if(unicode_glyphs[mid].cp<cp) low=mid+1;else high=mid;}
 if(low<sizeof unicode_glyphs/sizeof unicode_glyphs[0] && unicode_glyphs[low].cp==cp) return unicode_glyphs[low].pixels;
 return NULL;
}
static void draw_scalar(byte *p,int w,int h,int x,int y,uint32_t cp,byte color) {
 if(cp==9) cp='>';
 const byte *bitmap=unicode_bitmap(cp);
 if(bitmap) wtexc(p,w,w,h,(byte*)bitmap,x,y,FW,FH,color);
 else {wrectangulo(p,w,h,color,x+1,y+1,6,9);wbox(p,w,h,color,x+3,y+4,2,2);}
}
static void unicode_label(byte *p,int w,int h,int x,int y,const char *s,byte c,size_t limit) {
 size_t n=strlen(s),off=0;
 for(size_t i=0;off<n && i<limit;i++) {uint32_t cp=utf8_decode(s,n,&off);draw_scalar(p,w,h,x+(int)i*FW,y,cp,c);}
}
static void label(byte *p,int w,int h,int x,int y,const char *s,byte c) {
 int unicode=0;for(const unsigned char *t=(const unsigned char*)s;*t;t++) if(*t>=128) {unicode=1;break;}
 if(unicode) unicode_label(p,w,h,x,y,s,c,(size_t)(w-x)/FW);
 else wwrite(p,w,h,x,y,0,(byte*)s,c);
}
static int inside(int x,int y,SDL_Rect r) {return x>=r.x && y>=r.y && x<r.x+r.w && y<r.y+r.h;}
static int overlaps(SDL_Rect a,SDL_Rect b) {return a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;}
static SDL_Rect modal_rect(void) {
 if(pending) return (SDL_Rect){238,228,484,142};
 if(dialog<=2) return (SDL_Rect){100,80,760,440};
 if(dialog==5) return (SDL_Rect){140,120,680,400};
 return (SDL_Rect){140,220,680,170};
}
static void sync_modal(void) {
 int kind=pending?6:dialog;
 if(kind!=modal_kind) {modal_kind=kind;modal_opened=opened();modal_dx=modal_dy=modal_drag=help_drag=0;press_active=0;}
}
/* Measure actual font pixels; ellipsis never cuts a UTF-8 scalar. */
static void title_label(byte *dst,int w,int h,int x,int y,int width,const char *s) {
 char fitted[512];size_t n=strlen(s),p=0,used=0;int unicode=0;
 for(size_t i=0;i<n;i++) if((unsigned char)s[i]>=128) unicode=1;
 int available=width-4;
 while(p<n) {
  size_t next=utf8_next(s,n,p);if(next>=sizeof fitted-4) break;
  memcpy(fitted,s,next);fitted[next]=0;
  int pixels=unicode?(int)utf8_count(fitted,next)*FW:text_len((byte*)fitted);
  int reserve=next<n?(unicode?24:text_len((byte*)"...")):0;
  if(pixels+reserve>available) break;
  used=next;p=next;
 }
 memcpy(fitted,s,used);fitted[used]=0;
 if(used<n && available>=24) strcat(fitted,"...");
 int pixels=unicode?(int)utf8_count(fitted,strlen(fitted))*FW:text_len((byte*)fitted);
 int start=x+(width-pixels)/2;
 label(dst,w,h,start+1,y+1,fitted,c2);label(dst,w,h,start,y,fitted,10);
}
static void button(byte *dst,int w,int h,SDL_Rect r,const char *s) {
 int down=press_active && press_rect.x==r.x && press_rect.y==r.y && inside(mouse_x,mouse_y,r);
 /* Adapted from divwindo.c:wwrite_in_box's original bitmap-button frame. */
 wbox(dst,w,h,down?c1:c2,r.x,r.y,r.w,r.h);
 wrectangulo(dst,w,h,c0,r.x,r.y,r.w,r.h);
 wbox(dst,w,h,down?c0:c3,r.x+1,r.y+1,r.w-2,2);
 wbox(dst,w,h,down?c0:c3,r.x+1,r.y+1,2,r.h-2);
 wbox(dst,w,h,down?c2:c1,r.x+2,r.y+r.h-3,r.w-3,2);
 wbox(dst,w,h,down?c2:c1,r.x+r.w-3,r.y+2,2,r.h-3);
 wbox(dst,w,h,down?c0:c4,r.x+1,r.y+1,1,1);
 if(down) wbox(dst,w,h,c3,r.x+r.w-2,r.y+r.h-2,1,1);
 int tw=text_len((byte*)s);int x=r.x+(r.w-tw)/2+down,y=r.y+(r.h-14)/2+down;
 label(dst,w,h,x+1,y+1,s,c0);label(dst,w,h,x,y,s,down?c0:c4);
}
static int menu_dim(int index) {
 Menu *m=&menus[index];SDL_Rect r={m->x,m->y,m->w,m->h};
 for(int i=0;i<max_windows;i++) {struct tventana *w=&ventana[i];if(w->tipo && w->z>m->z && overlaps(r,(SDL_Rect){w->x,w->y,w->an,w->minimized?20:w->al})) return 1;}
 for(int i=0;i<5;i++) if(i!=index && menus[i].open && menus[i].z>m->z && overlaps(r,(SDL_Rect){menus[i].x,menus[i].y,menus[i].w,menus[i].h})) return 1;
 if(dialog || pending) {SDL_Rect d=modal_rect();d.x+=modal_dx;d.y+=modal_dy;if(overlaps(r,d)) return 1;}
 return 0;
}
static int window_dim(const struct tventana *win) {
 SDL_Rect r={win->x,win->y,win->an,win->minimized?20:win->al};
 for(int i=0;i<max_windows;i++) {const struct tventana *w=&ventana[i];
  if(w->tipo && w->z>win->z && overlaps(r,(SDL_Rect){w->x,w->y,w->an,w->minimized?20:w->al})) return 1;
 }
 for(int i=0;i<5;i++) {Menu *m=&menus[i];if(m->open && m->z>win->z && overlaps(r,(SDL_Rect){m->x,m->y,m->w,m->h})) return 1;}
 if(dialog || pending) {SDL_Rect d=modal_rect();d.x+=modal_dx;d.y+=modal_dy;if(overlaps(r,d)) return 1;}
 return 0;
}
void ui_draw_pointer(byte *dst,int x,int y) {
 if(x<0 || y<0 || x>=W || y>=H) return;
 int moving=dragging || menu_drag>=0 || modal_drag;
 /* GRAF_G.DIV control points: arrow (1,1), moving hand (2,1). */
 icon(dst,W,H,x-(moving?2:1),y-1,moving?2:1);
}
static size_t longest(Document *d) {
 size_t max=0,n=0;
 for(size_t i=0;i<=d->file_lon;i++) {
  if(d->buffer[i]=='\n' || !d->buffer[i]) {if(n>max) max=n;n=0;} else if(((unsigned char)d->buffer[i]&0xc0)!=0x80) n++;
 }
 return max+1;
}
static size_t max_first(size_t total,size_t visible) {return total>visible?total-visible+1:1;}
static void scroll(int horizontal,int delta) {
 Document *d=v.doc;size_t max=max_first(horizontal?longest(d):d->num_lineas,horizontal?cols():rows());
 size_t *first=horizontal?&d->primera_columna:&d->primera_linea;
 if(delta<0) {*first=*first>(size_t)-delta?*first-(size_t)-delta:1;}
 else {*first+=(size_t)delta;
 if(*first>max) *first=max;}
 d->scroll_free=1;
}
static void scroll_geometry(int horizontal,int *start,int *length,int *thumb,int *position) {
 size_t total=horizontal?longest(v.doc):v.doc->num_lineas;
 size_t visible=horizontal?cols():rows();
 size_t first=horizontal?v.doc->primera_columna:v.doc->primera_linea;
 *start=horizontal?36:52;*length=horizontal?v.an-72:v.al-88;
 *thumb=total>visible?(int)((size_t)*length*visible/total):*length;
 if(*thumb<10) *thumb=10;
 size_t max=max_first(total,visible);
 *position=*start+(max>1?(int)((first-1)*(size_t)(*length-*thumb)/(max-1)):0);
}
static void scroll_to(int horizontal,int coordinate) {
 int start,length,thumb,position;scroll_geometry(horizontal,&start,&length,&thumb,&position);
 int range=length-thumb,offset=coordinate-start-scroll_offset;
 if(offset<0) offset=0;
 if(offset>range) offset=range;
 size_t max=max_first(horizontal?longest(v.doc):v.doc->num_lineas,horizontal?cols():rows());
 size_t *first=horizontal?&v.doc->primera_columna:&v.doc->primera_linea;
 *first=range?1+(size_t)offset*(max-1)/(size_t)range:1;v.doc->scroll_free=1;
}
static void start_scroll(int horizontal,int coordinate) {
 int start,length,thumb,position;scroll_geometry(horizontal,&start,&length,&thumb,&position);
 scroll_offset=coordinate>=position && coordinate<position+thumb?coordinate-position:thumb/2;
 scroll_drag=horizontal?2:1;scroll_to(horizontal,coordinate);
}
static void scrollbars(void) {
 icon(v.ptr,v.an,v.al,v.an-18,36,39);icon(v.ptr,v.an,v.al,v.an-18,v.al-34,40);
 icon(v.ptr,v.an,v.al,20,v.al-18,51);icon(v.ptr,v.an,v.al,v.an-34,v.al-18,52);
 for(int h=0;h<2;h++) {
  int start,length,thumb,position;scroll_geometry(h,&start,&length,&thumb,&position);
  int x=h?start:v.an-18,y=h?v.al-18:start;
  wbox(v.ptr,v.an,v.al,c1,x,y,h?length:14,h?14:length);
  x=h?position:v.an-18;y=h?v.al-18:position;
  wbox(v.ptr,v.an,v.al,c2,x,y,h?thumb:14,h?14:thumb);
  wrectangulo(v.ptr,v.an,v.al,c3,x,y,h?thumb:14,h?14:thumb);
 }
}

static int resize_window(int x,int y,int w,int h) {
 if(w<320) w=320;
 if(h<180) h=180;
 if(w>W) w=W;
 if(h>H-68) h=H-68;
 if(x<0) x=0;
 if(y<38) y=38;
 if(x>W-w) x=W-w;
 if(y>H-30-h) y=H-30-h;
 byte *p=realloc(v.ptr,(size_t)w*h);if(!p) {v.doc->message="Resize failed: out of memory";return 0;}
 v.ptr=p;v.x=x;v.y=y;v.an=w;v.al=h;
 v.doc->primera_linea=1;v.doc->primera_columna=1;v.doc->scroll_free=0;return 1;
}
static void maximize(void) {
 if(!v.tipo) return;
 v.minimized=0;
 if(v.maximized) {if(resize_window(v.old_x,v.old_y,v.old_an,v.old_al)) v.maximized=0;}
 else {int x=v.x,y=v.y,w=v.an,h=v.al;if(resize_window(0,38,W,H-68)) {v.old_x=x;v.old_y=y;v.old_an=w;v.old_al=h;v.maximized=1;}}
}
static void paint(void) {
 Document *d=v.doc;char title[400],status[160];size_t a,b;
 if(d->primera_linea>max_first(d->num_lineas,rows())) d->primera_linea=max_first(d->num_lineas,rows());
 if(d->primera_columna>max_first(longest(d),cols())) d->primera_columna=max_first(longest(d),cols());
 document_view(d,cols(),rows());
 const char *name=d->path?(strrchr(d->path,'/')?strrchr(d->path,'/')+1:d->path):"Untitled";
 snprintf(title,sizeof title,"%s%s",name,d->modified?" *":"");
 wbox(v.ptr,v.an,v.al,c0,0,0,v.an,v.al);
 wrectangulo(v.ptr,v.an,v.al,c2,0,0,v.an,v.al);
 wgra(v.ptr,v.an,v.al,c_b_low,4,4,v.an-64,14);
 title_label(v.ptr,v.an,v.al,6,4,v.an-72,title);
 icon(v.ptr,v.an,v.al,v.an-18,4,35);
 icon(v.ptr,v.an,v.al,v.an-58,4,37);icon(v.ptr,v.an,v.al,v.an-38,4,56);
 wbox(v.ptr,v.an,v.al,c2,4,20,v.an-8,16);
 document_selection(d,&a,&b);
 snprintf(status,sizeof status,"%zu,%zu   INS  UTF-8   %zu lines   %zu selected",d->linea,d->columna,d->num_lineas,utf8_count(d->buffer+a,b-a));
 label(v.ptr,v.an,v.al,8,20,d->message?d->message:status,c4);
 wbox(v.ptr,v.an,v.al,5,4,37,v.an-24,v.al-60);
 size_t off=0,row=1;
 while(off<d->file_lon && row<d->primera_linea) if(d->buffer[off++]=='\n') row++;
 for(size_t y=0;y<rows() && off<=d->file_lon;y++) {
  size_t col=1;
  while(off<d->file_lon && d->buffer[off]!='\n') {
   size_t pos=off;uint32_t ch=utf8_decode(d->buffer,d->file_lon,&off);
   if(col>=d->primera_columna && col-d->primera_columna<cols()) {
    int x=TX+(int)(col-d->primera_columna)*FW,py=TY+(int)y*FH;
    int selected=pos>=a && pos<b;
    if(selected) wbox(v.ptr,v.an,v.al,19,x,py,FW,FH);
    draw_scalar(v.ptr,v.an,v.al,x,py,ch,c4);
   }
   col++;
  }
  if(off>=a && off<b && col>=d->primera_columna && col-d->primera_columna<cols())
   wbox(v.ptr,v.an,v.al,19,TX+(int)(col-d->primera_columna)*FW,TY+(int)y*FH,FW,FH);
  if(off>=d->file_lon) break;
  off++;
 }
 scrollbars();
 label(v.ptr,v.an,v.al,v.an-16,v.al-18,"/",c4);
}
static void nothing(void) {}
int ui_new(void) {
 if(ventana[max_windows-1].tipo) return 0;
 byte *p=calloc(720*480,1);
 if(!p) return 0;
 Document *doc=document_new();
 if(!doc) {free(p);return 0;}
 addwindow();memset(&v,0,sizeof(v));
 v.opened_at=opened();v.z=++zclock;v.tipo=102;v.orden=++serial;v.primer_plano=1;
 v.titulo="DIVseed / Untitled";v.an=720;v.al=480;v.doc=doc;
 v.x=208+(serial%3)*8;v.y=42+(serial%3)*16;
 v.ptr=p;v.paint_handler=paint;v.click_handler=nothing;v.close_handler=nothing;
 v.paint_handler();return 1;
}
void ui_close(void) {
 if(!v.tipo) return;
 v.close_handler();document_free(v.doc);free(v.ptr);divdelete(0);dragging=selecting=resizing=scroll_drag=0;
}
void ui_request_close(void) {
 if(v.tipo && v.doc->modified) {pending=1;dragging=selecting=resizing=scroll_drag=0;}
 else ui_close();
}
void ui_request_quit(void) {
 for(int i=0;i<max_windows;i++) if(ventana[i].tipo && ventana[i].doc->modified) {
  pending=2;dragging=selecting=resizing=scroll_drag=0;return;
 }
 quit_ready=1;
}
int ui_quit_ready(void) {return quit_ready;}
int ui_confirmation(void) {return pending;}
static void confirm(int discard) {
 int action=pending;pending=0;
 if(discard) {if(action==1) ui_close();else if(action==2) quit_ready=1;}
}
static void clipboard(int cut) {
 size_t a,b;
 if(!document_selection(v.doc,&a,&b)) {v.doc->message="Select text first";return;}
 char *s=document_copy(v.doc);
 if(!s) {v.doc->message="Out of memory: clipboard unchanged";return;}
 if(SDL_SetClipboardText(s)) v.doc->message="Clipboard unavailable: text unchanged";
 else if(cut) document_delete_selection(v.doc);
 else v.doc->message="Selection copied";
 free(s);
}
static void paste(void) {
 char *s=SDL_GetClipboardText();
 if(!s) v.doc->message="Clipboard unavailable";
 else {document_paste(v.doc,s);SDL_free(s);}
}
static void point(int x,int y,int extend) {
 if(y<TY && v.doc->primera_linea>1) scroll(0,-1);
 if(y>=TY+(int)rows()*FH) scroll(0,1);
 if(x<TX && v.doc->primera_columna>1) scroll(1,-1);
 if(x>=TX+(int)cols()*FW) scroll(1,1);
 int col=(x-TX)/FW,row=(y-TY)/FH;
 if(col<0) col=0;
 if(row<0) row=0;
 if(col>=(int)cols()) col=(int)cols()-1;
 if(row>=(int)rows()) row=(int)rows()-1;
 document_point(v.doc,v.doc->primera_linea+(size_t)row,v.doc->primera_columna+(size_t)col,extend);
}
static void browser_scroll(int files,int delta) {
 size_t *first=files?&browser.file_first:&browser.dir_first;
 size_t count=files?browser.nfiles:browser.ndirs,visible=files?36:12;
 size_t max=count>visible?count-visible:0;
 if(files) max=(max+2)/3*3;
 size_t step=(size_t)(delta<0?-delta:delta)*(files?3:1);
 if(delta<0) *first=*first>step?*first-step:0;
 else *first=*first+step>max?max:*first+step;
}
static void browser_bar(byte *dst,int files) {
 int x=files?836:292;size_t count=files?browser.nfiles:browser.ndirs;
 size_t first=files?browser.file_first:browser.dir_first,visible=files?36:12;
 wbox(dst,W,H,c1,x,146,14,264);icon(dst,W,H,x,146,39);icon(dst,W,H,x,396,40);
 int thumb=count>visible?(int)(236*visible/count):236;if(thumb<12) thumb=12;
 size_t max=count>visible?count-visible:0;if(files) max=(max+2)/3*3;
 int position=160+(max?(int)(first*(size_t)(236-thumb)/max):0);
 wbox(dst,W,H,c2,x,position,14,thumb);wrectangulo(dst,W,H,c3,x,position,14,thumb);
}
static void start_path(int mode) {
 dialog=mode;overwrite=0;notice[0]=0;selected_file=-1;
 if(!browser.path[0]) browser_read(&browser,".");
 else browser_read(&browser,browser.path);
 snprintf(filename,sizeof filename,"%s",mode==2 && v.tipo && v.doc->path?v.doc->path:"");
}
static void save_done(void) {
 int action=after_save;after_save=0;
 if(action==1) ui_close();
 else if(action==2) ui_request_quit();
}
static void save_current(int as) {
 if(!v.tipo) return;
 if(as || !v.doc->path) {start_path(2);return;}
 if(document_save(v.doc,v.doc->path,1)==1) save_done();
 else {snprintf(notice,sizeof notice,"%s",v.doc->message);dialog=4;after_save=0;}
}
static void submit_path(void) {
 if(!*filename) {snprintf(notice,sizeof notice,"Choose a file or enter a file name");return;}
 char path[4096];
 if(!browser_join(&browser,filename,path,sizeof path)) {snprintf(notice,sizeof notice,"Path too long");return;}
 struct stat st;
 if(!stat(path,&st) && S_ISDIR(st.st_mode)) {
  if(browser_read(&browser,path)) {filename[0]=notice[0]=0;selected_file=-1;}
  else snprintf(notice,sizeof notice,"%s",browser.error);
  return;
 }
 if(dialog==1) {
  const char *error;Document *d=document_open(path,&error);
  if(!d) {snprintf(notice,sizeof notice,"%s",error);return;}
  if(!ui_new()) {document_free(d);snprintf(notice,sizeof notice,"Cannot create another window");return;}
  document_free(v.doc);v.doc=d;dialog=0;
 } else {
  int result=document_save(v.doc,path,overwrite);
  if(result==1) {dialog=0;save_done();}
  else {snprintf(notice,sizeof notice,"%s",v.doc->message);if(result==2) overwrite=1;}
 }
}
static void menu_action(int m,int row) {
 if(m==0) {if(row==4) {ui_request_quit();return;}if(!menus[row+1].open) menus[row+1].opened_at=opened();menus[row+1].open=1;menus[row+1].z=++zclock;return;}
 if(m==1) switch(row) {
  case 0:ui_new();break;case 1:start_path(1);break;case 2:save_current(0);break;
  case 3:save_current(1);break;case 4:ui_request_close();break;case 5:ui_request_quit();break;
 }
 if(m==2 && v.tipo) switch(row) {
  case 0:document_undo(v.doc);break;case 1:document_redo(v.doc);break;
  case 2:clipboard(1);break;case 3:clipboard(0);break;case 4:paste();break;case 5:document_select_all(v.doc);break;
 }
 if(m==3 && v.tipo) {if(row==0) v.minimized=!v.minimized;else maximize();}
 if(m==4) {dialog=row?3:5;help_first=0;}
}
static int menu_hit(int x,int y,int window_z) {
 int hit=-1,z=window_z;
 for(int i=0;i<5;i++) {Menu *m=&menus[i];if(m->open && m->z>=z && x>=m->x && x<m->x+m->w && y>=m->y && y<m->y+m->h) {hit=i;z=m->z;}}
 if(hit<0) return 0;
 Menu *m=&menus[hit];m->z=++zclock;
 if(y<m->y+20) {
  if(hit && x>=m->x+m->w-20) m->open=0;
  else {menu_drag=hit;dx=x-m->x;dy=y-m->y;}
 } else {int row=(y-m->y-24)/22;if(y>=m->y+24 && row<m->count) menu_action(hit,row);}
 return 1;
}
static void draw_menu(byte *dst,int index) {
 Menu *m=&menus[index];if(opening(dst,(SDL_Rect){m->x,m->y,m->w,m->h},m->opened_at)) return;
 int dim=menu_dim(index);byte fg=9;
 wbox(dst,W,H,dim?12:8,m->x,m->y,m->w,m->h);wrectangulo(dst,W,H,menu_drag==index?10:c2,m->x,m->y,m->w,m->h);
 if(dim) wbox(dst,W,H,c1,m->x+3,m->y+3,m->w-6,16);
 else wgra(dst,W,H,c_b_low,m->x+3,m->y+3,m->w-6,16);
 title_label(dst,W,H,m->x+4,m->y+3,m->w-(index?26:8),m->title);
 if(index) label(dst,W,H,m->x+m->w-16,m->y+3,"x",fg);
 int hover=mouse_x>=m->x && mouse_x<m->x+m->w && mouse_y>=m->y+24 && mouse_y<m->y+m->h;
 if(dialog || pending) hover=0;
 for(int j=0;j<max_windows;j++) {struct tventana *w=&ventana[j];if(w->tipo && w->z>m->z && mouse_x>=w->x && mouse_x<w->x+w->an && mouse_y>=w->y && mouse_y<w->y+(w->minimized?20:w->al)) hover=0;}
 for(int j=0;j<5;j++) {Menu *other=&menus[j];if(other->open && other->z>m->z && mouse_x>=other->x && mouse_x<other->x+other->w && mouse_y>=other->y && mouse_y<other->y+other->h) hover=0;}
 for(int i=0;i<m->count;i++) {
  int selected=hover && (mouse_y-m->y-24)/22==i;
  if(selected) label(dst,W,H,m->x+11,m->y+25+i*22,m->items[i],11);
  label(dst,W,H,m->x+10,m->y+24+i*22,m->items[i],selected?10:fg);
 }
}

int ui_init(void) {
 char *fontbase=SDL_GetBasePath();
 if(!fontbase) return 0;
 char fontpath[4096];
 snprintf(fontpath,sizeof fontpath,"%sassets/SYS08X11.BIN",fontbase); SDL_free(fontbase);
 FILE *ef=fopen(fontpath,"rb"); if(!ef) return 0;
 size_t nr=fread(editor_font,1,sizeof editor_font,ef); fclose(ef);
 if(nr!=sizeof editor_font) return 0;
 char *base=SDL_GetBasePath();
 if(!base) return 0;
 size_t n=strlen(base)+32; char *path=malloc(n);
 if(!path) { SDL_free(base); return 0; }
 snprintf(path,n,"%sassets/grande.fon",base); SDL_free(base);
 FILE *f=fopen(path,"rb"); free(path);
 if(!f) return 0;
 if(fseek(f,0,SEEK_END)) { fclose(f); return 0; }
 long size=ftell(f); rewind(f);
 if(size<1025 || size>65536) { fclose(f); return 0; }
 text_font=malloc((size_t)size);
 if(!text_font) { fclose(f); return 0; }
 size_t got=fread(text_font,1,(size_t)size,f); fclose(f);
 if(got!=(size_t)size || text_font[0]!=14) goto fail;
 for(int i=0;i<256;i++) {
  const byte *p=text_font+1+4*i;
  glyphs[i].an=p[0]; glyphs[i].dir=(word)(p[2]|p[3]<<8);
  if(!glyphs[i].an || 1025+glyphs[i].dir+14*glyphs[i].an>size) goto fail;
 }
 cbn=cgn=crn=8;
 for(int i=0;i<8;i++) { cbs[i]=16+i; cgs[i]=1+i%4; crs[i]=32+i; }
 return 1;
fail: free(text_font); text_font=NULL; return 0;
}static void handle_event(const SDL_Event *e) {
 if(e->type==SDL_MOUSEMOTION) {mouse_x=e->motion.x;mouse_y=e->motion.y;}
 if(e->type==SDL_MOUSEBUTTONDOWN) {mouse_x=e->button.x;mouse_y=e->button.y;}
 if(e->type==SDL_KEYUP) suppress_text=0;
 if(e->type==SDL_TEXTINPUT && suppress_text) return;
 if(dialog) {
  if(e->type==SDL_KEYDOWN) {
   SDL_Keycode k=e->key.keysym.sym;
   if(k==SDLK_ESCAPE) {dialog=after_save=0;}
   else if(k==SDLK_RETURN) {if(dialog<=2) submit_path();else dialog=0;}
   else if(dialog==5) {
    if(k==SDLK_DOWN) help_first++;else if(k==SDLK_UP) help_first--;
    else if(k==SDLK_PAGEDOWN) help_first+=HELP_ROWS;else if(k==SDLK_PAGEUP) help_first-=HELP_ROWS;
    else if(k==SDLK_HOME) help_first=0;else if(k==SDLK_END) help_first=HELP_COUNT-HELP_ROWS;
    if(help_first<0) help_first=0;
    if(help_first>HELP_COUNT-HELP_ROWS) help_first=HELP_COUNT-HELP_ROWS;
   }
   else if(k==SDLK_a && (e->key.keysym.mod&KMOD_CTRL) && dialog<=2) {filename[0]=0;overwrite=0;}
   else if((k==SDLK_UP || k==SDLK_DOWN) && dialog<=2 && browser.nfiles) {
    if(selected_file<0) selected_file=0;else if(k==SDLK_UP && selected_file>0) selected_file--;else if(k==SDLK_DOWN && (size_t)(selected_file+1)<browser.nfiles) selected_file++;
    snprintf(filename,sizeof filename,"%s",browser.files[selected_file].name);overwrite=0;
    if((size_t)selected_file<browser.file_first) browser.file_first=(size_t)selected_file/3*3;
    if((size_t)selected_file>=browser.file_first+36) browser.file_first=((size_t)selected_file/3-11)*3;
   }
   else if(k==SDLK_BACKSPACE && dialog<=2) {size_t n=strlen(filename);if(n) filename[utf8_prev(filename,n)]=0;overwrite=0;}
  } else if(e->type==SDL_TEXTINPUT && dialog<=2) {
   size_t n=strlen(filename),add=strlen(e->text.text);
   if(n+add<sizeof filename && utf8_valid(e->text.text,add,0)) {memcpy(filename+n,e->text.text,add+1);overwrite=0;notice[0]=0;}
  } else if(e->type==SDL_MOUSEWHEEL && dialog==5) {
   help_first-=e->wheel.y*3;if(help_first<0) help_first=0;
    if(help_first>HELP_COUNT-HELP_ROWS) help_first=HELP_COUNT-HELP_ROWS;
  } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT && dialog>=3) {
   int x=e->button.x,y=e->button.y;
   if(dialog==5) {
    if(inside(x,y,(SDL_Rect){690,480,110,28})) dialog=0;
    else if(x>=786 && x<802 && y>=160 && y<460) {
     if(y<176) help_first--;else if(y>=444) help_first++;
     else {help_drag=1;help_first=(y-176)*(HELP_COUNT-HELP_ROWS)/267;}
     if(help_first<0) help_first=0;
    if(help_first>HELP_COUNT-HELP_ROWS) help_first=HELP_COUNT-HELP_ROWS;
    }
   } else if(inside(x,y,(SDL_Rect){670,338,130,28})) dialog=0;
  } else if(e->type==SDL_MOUSEMOTION && dialog==5 && help_drag) {
   help_first=(e->motion.y-176)*(HELP_COUNT-HELP_ROWS)/267;
   if(help_first<0) help_first=0;
    if(help_first>HELP_COUNT-HELP_ROWS) help_first=HELP_COUNT-HELP_ROWS;
  } else if(e->type==SDL_MOUSEWHEEL && dialog<=2) {
   browser_scroll(mouse_x>=306,-e->wheel.y*3);
  } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT && dialog<=2) {
   int x=e->button.x,y=e->button.y;
   if(x>=828 && y>=84 && y<106) {dialog=after_save=0;}
   else if(y>=480 && y<510) {if(x>=740) dialog=after_save=0;else if(x>=620) submit_path();}
   else if(((x>=292 && x<306) || (x>=836 && x<850)) && y>=146 && y<410) {
    int files=x>=836;
    if(y<160) browser_scroll(files,-1);else if(y>=396) browser_scroll(files,1);
    else {size_t count=files?browser.nfiles:browser.ndirs,visible=files?36:12;
     size_t max=count>visible?count-visible:0;if(files) max=(max+2)/3*3;
     size_t first=(size_t)(y-160)*max/235;if(files) first=first/3*3;
     if(files) browser.file_first=first;else browser.dir_first=first;
    }
   }
   else if(x>=112 && x<290 && y>=146 && y<410) {
    size_t i=browser.dir_first+(size_t)(y-146)/22;
    if(i<browser.ndirs) {char path[4096];if(browser_join(&browser,browser.dirs[i].name,path,sizeof path) && browser_read(&browser,path)) {filename[0]=notice[0]=0;selected_file=-1;overwrite=0;}}
   } else if(x>=310 && x<832 && y>=146 && y<410) {
    size_t i=browser.file_first+(size_t)(y-146)/22*3+(size_t)(x-310)/174;
    if(i<browser.nfiles) {selected_file=(int)i;snprintf(filename,sizeof filename,"%s",browser.files[i].name);overwrite=0;notice[0]=0;if(e->button.clicks>=2) submit_path();}
   } else if(y>=416 && y<440 && x>=112 && x<290) {browser.text_only=!browser.text_only;browser_read(&browser,browser.path);selected_file=-1;}
  }
  return;
 }
 if(pending) {
  if(e->type==SDL_KEYDOWN) {
   if(e->key.keysym.sym==SDLK_ESCAPE || e->key.keysym.sym==SDLK_RETURN) confirm(0);
   else if(e->key.keysym.sym==SDLK_s) {
    suppress_text=1;after_save=pending;pending=0;
    if(after_save==2) for(int i=0;i<max_windows;i++) if(ventana[i].tipo && ventana[i].doc->modified) {move(0,i);break;}
    save_current(0);
   }
   else if(e->key.keysym.sym==SDLK_d) {suppress_text=1;confirm(1);}
  } else if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT && e->button.y>=326 && e->button.y<356) {
   if(e->button.x>=270 && e->button.x<490) confirm(0);
   else if(e->button.x>=540 && e->button.x<690) confirm(1);
  }
  return;
 }
 if(e->type==SDL_QUIT) {ui_request_quit();return;}
 if(e->type==SDL_TEXTINPUT && v.tipo && !v.minimized) document_text(v.doc,e->text.text);
 if(e->type==SDL_KEYDOWN) {
  SDL_Keycode code=e->key.keysym.sym;
  if(code==SDLK_F1) {dialog=5;help_first=0;return;}
  if(code==SDLK_ESCAPE) {ui_request_quit();return;}
  if(e->key.keysym.mod & KMOD_CTRL) {
   if(code==SDLK_o) start_path(1);else if(code==SDLK_s) save_current((e->key.keysym.mod & KMOD_SHIFT)!=0);else if(code==SDLK_n) ui_new();else if(code==SDLK_w) ui_request_close();
   else if(v.tipo) switch(code) {
    case SDLK_a:document_select_all(v.doc);break;
    case SDLK_c:clipboard(0);break;case SDLK_x:clipboard(1);break;case SDLK_v:paste();break;
    case SDLK_z:if(e->key.keysym.mod & KMOD_SHIFT) document_redo(v.doc);else document_undo(v.doc);break;
    case SDLK_y:document_redo(v.doc);break;default:break;
   }
  } else if(v.tipo && !v.minimized) {
   EditKey key;int handled=1;
   switch(code) {
    case SDLK_LEFT:key=ED_LEFT;break;case SDLK_RIGHT:key=ED_RIGHT;break;
    case SDLK_UP:key=ED_UP;break;case SDLK_DOWN:key=ED_DOWN;break;
    case SDLK_HOME:key=ED_HOME;break;case SDLK_END:key=ED_END;break;
    case SDLK_BACKSPACE:key=ED_BACKSPACE;break;case SDLK_DELETE:key=ED_DELETE;break;
    case SDLK_TAB:document_text(v.doc,"\t");handled=0;break;
    case SDLK_RETURN:case SDLK_KP_ENTER:key=ED_ENTER;break;default:handled=0;break;
   }
   if(handled) {
    if(key==ED_BACKSPACE || key==ED_DELETE || key==ED_ENTER) document_key(v.doc,key);
    else document_move(v.doc,key,(e->key.keysym.mod & KMOD_SHIFT)!=0);
   }
  }
 }
 if(e->type==SDL_MOUSEWHEEL && v.tipo && !v.minimized) scroll(0,-e->wheel.y*3);
 if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT) {
  int x=e->button.x,y=e->button.y;
  if(y<36) {if(x>=120 && x<210) dialog=3;return;}
  int wz=-1;
  for(int i=0;i<max_windows;i++) {struct tventana *w=&ventana[i];if(w->tipo && x>=w->x && x<w->x+w->an && y>=w->y && y<w->y+(w->minimized?20:w->al)) {wz=w->z;break;}}
  if(menu_hit(x,y,wz)) return;
  for(int i=0;i<max_windows;i++) {
   struct tventana *win=&ventana[i];
   if(!win->tipo || x<win->x || y<win->y || x>=win->x+win->an || y>=win->y+(win->minimized?20:win->al)) continue;
   move(0,i);v.z=++zclock;x-=v.x;y-=v.y;
   if(y<20) {
    if(x>=v.an-22) ui_request_close();
    else if(x>=v.an-42) maximize();
    else if(x>=v.an-62) v.minimized=!v.minimized;
    else if(v.minimized) v.minimized=0;
    else if(!v.maximized) {dragging=1;dx=x;dy=y;}
   } else if(x>=v.an-20 && y>=v.al-20 && !v.maximized) {resizing=1;}
   else if(x>=v.an-20 && y>=36 && y<v.al-20) {
    if(y<52) scroll(0,-1);else if(y>=v.al-36) scroll(0,1);
    else start_scroll(0,y);
   } else if(y>=v.al-20) {
    if(x<36) scroll(1,-1);else if(x>=v.an-36) scroll(1,1);
    else start_scroll(1,x);
   } else if(y>=TY && x<v.an-20) {point(x,y,(SDL_GetModState()&KMOD_SHIFT)!=0);selecting=1;}
   break;
  }
 }
 if(e->type==SDL_MOUSEBUTTONUP || (e->type==SDL_WINDOWEVENT && e->window.event==SDL_WINDOWEVENT_FOCUS_LOST)) dragging=selecting=resizing=scroll_drag=0,menu_drag=-1;
 if(e->type==SDL_MOUSEMOTION && menu_drag>=0) {
  Menu *m=&menus[menu_drag];m->x=e->motion.x-dx;m->y=e->motion.y-dy;
  if(m->x<0) m->x=0;
  if(m->x>W-m->w) m->x=W-m->w;
  if(m->y<38) m->y=38;
  if(m->y>H-30-m->h) m->y=H-30-m->h;
  return;
 }
 if(e->type==SDL_MOUSEMOTION && v.tipo) {
  if(resizing) {
   int width=e->motion.x-v.x+1;if(width>W-v.x) width=W-v.x;
   int h=e->motion.y-v.y+1;if(h>H-30-v.y) h=H-30-v.y;
   resize_window(v.x,v.y,width,h);
  } else if(scroll_drag) {
   scroll_to(scroll_drag==2,scroll_drag==2?e->motion.x-v.x:e->motion.y-v.y);
  } else if(dragging) {
   v.x=e->motion.x-dx;v.y=e->motion.y-dy;
   if(v.x<0) v.x=0;
 if(v.x>W-v.an) v.x=W-v.an;
   if(v.y<38) v.y=38;
 if(v.y>H-30-v.al) v.y=H-30-v.al;
  } else if(selecting) point(e->motion.x-v.x,e->motion.y-v.y,1);
 }
}
/* Controls activate on release inside their original bounds. A drag-out cancels. */
static int button_at(int x,int y,SDL_Rect *out) {
 SDL_Rect candidates[12];int n=0;
 if(pending) {candidates[n++]=(SDL_Rect){270,326,220,30};candidates[n++]=(SDL_Rect){540,326,150,30};}
 else if(dialog) {
  if(dialog<=2) {
   candidates[n++]=(SDL_Rect){620,480,110,30};candidates[n++]=(SDL_Rect){740,480,110,30};candidates[n++]=(SDL_Rect){838,86,14,14};
   candidates[n++]=(SDL_Rect){292,146,14,14};candidates[n++]=(SDL_Rect){292,396,14,14};
   candidates[n++]=(SDL_Rect){836,146,14,14};candidates[n++]=(SDL_Rect){836,396,14,14};
  } else if(dialog==5) {
   candidates[n++]=(SDL_Rect){690,480,110,28};candidates[n++]=(SDL_Rect){786,160,14,16};candidates[n++]=(SDL_Rect){786,444,14,16};
  } else candidates[n++]=(SDL_Rect){670,338,130,28};
 } else if(y<36) candidates[n++]=(SDL_Rect){120,5,90,26};
 else {
  int win=-1,menu=-1,z=-1;
  for(int i=0;i<max_windows;i++) {struct tventana *w=&ventana[i];if(w->tipo && w->z>z && inside(x,y,(SDL_Rect){w->x,w->y,w->an,w->minimized?20:w->al})) {win=i;z=w->z;}}
  for(int i=0;i<5;i++) {Menu *m=&menus[i];if(m->open && m->z>z && inside(x,y,(SDL_Rect){m->x,m->y,m->w,m->h})) {menu=i;win=-1;z=m->z;}}
  if(menu>0) candidates[n++]=(SDL_Rect){menus[menu].x+menus[menu].w-20,menus[menu].y,20,20};
  else if(win>=0) {
   struct tventana *w=&ventana[win];
   candidates[n++]=(SDL_Rect){w->x+w->an-22,w->y,22,20};
   candidates[n++]=(SDL_Rect){w->x+w->an-42,w->y,20,20};
   candidates[n++]=(SDL_Rect){w->x+w->an-62,w->y,20,20};
   if(!w->minimized) {
    candidates[n++]=(SDL_Rect){w->x+w->an-20,w->y+36,20,16};candidates[n++]=(SDL_Rect){w->x+w->an-20,w->y+w->al-36,20,16};
    candidates[n++]=(SDL_Rect){w->x+20,w->y+w->al-20,16,20};candidates[n++]=(SDL_Rect){w->x+w->an-36,w->y+w->al-20,16,20};
   }
  }
 }
 for(int i=0;i<n;i++) if(inside(x,y,candidates[i])) {*out=candidates[i];return 1;}
 return 0;
}
void ui_event(const SDL_Event *input) {
 sync_modal();SDL_Event mapped=*input;
 int is_modal=dialog || pending;
 if(input->type==SDL_WINDOWEVENT && input->window.event==SDL_WINDOWEVENT_FOCUS_LOST) {press_active=modal_drag=help_drag=0;handle_event(input);return;}
 if(input->type==SDL_KEYDOWN) press_active=0;
 if(is_modal) {
  if(input->type==SDL_MOUSEMOTION) {
   if(modal_drag) {
    SDL_Rect r=modal_rect();int x=input->motion.x-dx,y=input->motion.y-dy;
    if(x<0) x=0;
    if(x>W-r.w) x=W-r.w;
    if(y<38) y=38;
    if(y>H-r.h) y=H-r.h;
    modal_dx=x-r.x;modal_dy=y-r.y;return;
   }
   mapped.motion.x-=modal_dx;mapped.motion.y-=modal_dy;
  } else if(input->type==SDL_MOUSEBUTTONDOWN || input->type==SDL_MOUSEBUTTONUP) {mapped.button.x-=modal_dx;mapped.button.y-=modal_dy;}
 }
 const SDL_Event *e=&mapped;
 if(e->type==SDL_MOUSEMOTION) {mouse_x=e->motion.x;mouse_y=e->motion.y;if(press_active) return;}
 if(e->type==SDL_MOUSEBUTTONDOWN && e->button.button==SDL_BUTTON_LEFT) {
  mouse_x=e->button.x;mouse_y=e->button.y;
  if(button_at(mouse_x,mouse_y,&press_rect)) {press_active=1;return;}
  if(is_modal) {
   SDL_Rect r=modal_rect();
   if(inside(mouse_x,mouse_y,(SDL_Rect){r.x+4,r.y+4,r.w-8,22})) {modal_drag=1;dx=input->button.x-(r.x+modal_dx);dy=input->button.y-(r.y+modal_dy);return;}
  }
 }
 if(e->type==SDL_MOUSEBUTTONUP && e->button.button==SDL_BUTTON_LEFT) {
  modal_drag=help_drag=0;
  if(press_active) {
   press_active=0;
   if(inside(e->button.x,e->button.y,press_rect)) {SDL_Event click=*e;click.type=SDL_MOUSEBUTTONDOWN;handle_event(&click);}
   handle_event(e);return;
  }
 }
 handle_event(e);
}

void ui_draw(byte *dst) {
 sync_modal();
 for(int i=0;i<max_windows;i++) if(ventana[i].tipo) {
  if(i) {copy(-1,0);copy(0,i);} v.paint_handler();
  if(i) {copy(i,0);copy(0,-1);}
 }
 if(v.tipo && !v.minimized && !pending && !dialog) {
  Document *d=v.doc;
  if(d->columna>=d->primera_columna && d->columna-d->primera_columna<cols() &&
     d->linea>=d->primera_linea && d->linea-d->primera_linea<rows())
   wbox(v.ptr,v.an,v.al,c4,TX+(int)(d->columna-d->primera_columna)*FW,
        TY+(int)(d->linea-d->primera_linea)*FH,1,FH);
 }
 wbox(dst,W,H,6,0,0,W,H);
 for(int y=38;y<H-30;y+=4) for(int x=(y%8)/2;x<W;x+=4) dst[y*W+x]=7;
 wbox(dst,W,H,c1,0,0,W,36);
 label(dst,W,H,4,10,"DIVseed",c4);button(dst,W,H,(SDL_Rect){120,5,90,26},"About");
 wbox(dst,W,H,c1,0,H-28,W,28);
 label(dst,W,H,10,H-20,"Shift: select   Ctrl+C/X/V   Ctrl+Z/Y   Ctrl+O: Open   Ctrl+S: Save",c4);
 /* Merge menu and document layers in activation order. */
 int drawn_menu[5]={0},drawn_window[max_windows]={0};
 for(int layer=0;layer<max_windows+5;layer++) {
  int next=-1,is_menu=0,z=2147483647;
  for(int i=0;i<max_windows;i++) if(ventana[i].tipo && !drawn_window[i] && ventana[i].z<z) {next=i;z=ventana[i].z;is_menu=0;}
  for(int i=0;i<5;i++) if(menus[i].open && !drawn_menu[i] && menus[i].z<z) {next=i;z=menus[i].z;is_menu=1;}
  if(next<0) break;
  if(is_menu) {draw_menu(dst,next);drawn_menu[next]=1;continue;}
  struct tventana *win=&ventana[next];drawn_window[next]=1;
  int height=win->minimized?20:win->al;
  if(opening(dst,(SDL_Rect){win->x,win->y,win->an,height},win->opened_at)) continue;
  int dim=window_dim(win);
  for(int y=0;y<height;y++) for(int x=0;x<win->an;x++) {
   byte pixel=win->ptr[y*win->an+x];dst[(win->y+y)*W+win->x+x]=dim?(byte)(pixel+64):pixel;
  }
  wrectangulo(dst,W,H,next==0 && dragging?10:c2,win->x,win->y,win->an,height);
 }
 byte *desktop=dst;
 if(dialog || pending) dst=modal_pixels;
 if(dialog && dialog<=2) {
  wbox(dst,W,H,8,100,80,760,440);wrectangulo(dst,W,H,c3,100,80,760,440);
  wgra(dst,W,H,c_b_low,104,84,752,20);title_label(dst,W,H,110,86,718,dialog==1?"Open file...":"Save file as...");icon(dst,W,H,838,86,35);
  wbox(dst,W,H,c1,110,110,740,20);unicode_label(dst,W,H,114,114,browser.path,c4,90);
  label(dst,W,H,112,132,"Directory:",c4);label(dst,W,H,310,132,"Files:",c4);
  wbox(dst,W,H,c1,110,146,184,264);wrectangulo(dst,W,H,c0,110,146,184,264);
  for(size_t row=0;row<12;row++) {
   size_t i=browser.dir_first+row;
   if(i<browser.ndirs) unicode_label(dst,W,H,114,150+(int)row*22,browser.dirs[i].name,c4,21);
   for(size_t col=0;col<3;col++) {
    i=browser.file_first+row*3+col;int x=310+(int)col*174,y=146+(int)row*22;
    wbox(dst,W,H,(int)i==selected_file?10:5,x,y,172,21);
    if(i<browser.nfiles) unicode_label(dst,W,H,x+4,y+4,browser.files[i].name,(int)i==selected_file?11:c4,20);
   }
  }
  browser_bar(dst,0);browser_bar(dst,1);
  label(dst,W,H,112,418,browser.text_only?"Type: *.txt":"Type: *.*",c4);
  label(dst,W,H,310,418,"Wheel: scroll | Up/Down: select | Enter: accept",c4);
  unicode_label(dst,W,H,112,444,notice[0]?notice:browser.error?browser.error:"",c4,90);
  label(dst,W,H,112,466,"Filename:",c4);wbox(dst,W,H,c1,110,484,490,22);
  size_t n=strlen(filename),chars=utf8_count(filename,n);size_t start=utf8_offset(filename,n,chars>59?chars-59:0);
  unicode_label(dst,W,H,114,488,filename+start,c4,59);
  button(dst,W,H,(SDL_Rect){620,480,110,30},overwrite?"Replace":"Accept");button(dst,W,H,(SDL_Rect){740,480,110,30},"Cancel");
 } else if(dialog==5) {
  wbox(dst,W,H,8,140,120,680,400);wrectangulo(dst,W,H,c4,140,120,680,400);
  wgra(dst,W,H,c_b_low,144,124,672,20);title_label(dst,W,H,150,126,660,"Help - commands and options");
  wbox(dst,W,H,5,154,158,628,306);
  for(int i=0;i<HELP_ROWS && help_first+i<HELP_COUNT;i++) unicode_label(dst,W,H,164,164+i*24,help_lines[help_first+i],c4,75);
  wbox(dst,W,H,c1,786,160,16,300);icon(dst,W,H,786,160,39);icon(dst,W,H,786,446,40);
  int thumb=268*HELP_ROWS/HELP_COUNT;int position=176+help_first*(268-thumb)/(HELP_COUNT-HELP_ROWS);
  wbox(dst,W,H,c2,786,position,14,thumb);wrectangulo(dst,W,H,c3,786,position,14,thumb);
  label(dst,W,H,160,486,"Wheel / arrows / Page Up / Page Down",c4);button(dst,W,H,(SDL_Rect){690,480,110,28},"OK");
 } else if(dialog) {
  wbox(dst,W,H,8,140,220,680,170);wrectangulo(dst,W,H,c4,140,220,680,170);
  wgra(dst,W,H,c_b_low,144,224,672,20);
  title_label(dst,W,H,150,226,660,dialog==3?"About DIVseed":"File operation");
  label(dst,W,H,156,266,dialog==3?"DIVseed alpha 0.1 - derived from DIV Games Studio":notice,c4);
  label(dst,W,H,156,292,dialog==3?"GPLv3 | Original graphics | UTF-8 scalar editing":"",c4);
  label(dst,W,H,156,346,"Enter / Esc: close",c4);button(dst,W,H,(SDL_Rect){670,338,130,28},"OK / Accept");
 }
 if(pending) {
  wbox(dst,W,H,8,238,228,484,142);wrectangulo(dst,W,H,c4,238,228,484,142);
  wgra(dst,W,H,c_b_low,242,232,476,18);
  title_label(dst,W,H,248,234,464,"Unsaved text");
  label(dst,W,H,254,270,pending==2?"Discard ALL documents and quit?":"Discard this document?",c4);
  label(dst,W,H,254,292,"S: Save first   Enter: cancel   D: discard",c3);
  button(dst,W,H,(SDL_Rect){270,326,220,30},"Keep editing");
  button(dst,W,H,(SDL_Rect){540,326,150,30},"Discard");
 }
 if(dialog || pending) {
  SDL_Rect r=modal_rect();
  if(!opening(desktop,(SDL_Rect){r.x+modal_dx,r.y+modal_dy,r.w,r.h},modal_opened))
  for(int y=0;y<r.h;y++) memcpy(desktop+(r.y+modal_dy+y)*W+r.x+modal_dx,dst+(r.y+y)*W+r.x,(size_t)r.w);
 }
 if(press_active && press_rect.w<=22 && inside(mouse_x,mouse_y,press_rect)) {
  SDL_Rect r=press_rect;if(dialog || pending) {r.x+=modal_dx;r.y+=modal_dy;}
  wbox(desktop,W,H,c0,r.x,r.y,r.w,1);wbox(desktop,W,H,c0,r.x,r.y,1,r.h);
  wbox(desktop,W,H,c4,r.x,r.y+r.h-1,r.w,1);wbox(desktop,W,H,c4,r.x+r.w-1,r.y,1,r.h);
 }
}
void ui_destroy(void) {press_active=modal_kind=modal_dx=modal_dy=modal_drag=help_drag=help_first=0;browser_free(&browser);memset(&browser,0,sizeof browser);while(v.tipo) ui_close();free(text_font);text_font=NULL;pending=quit_ready=suppress_text=0;}
