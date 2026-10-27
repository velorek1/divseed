#ifndef DIVSEED_UI_H
#define DIVSEED_UI_H
#include "editor.h"
extern int ui_width,ui_height;
#define UI_WIDTH ui_width
#define UI_HEIGHT ui_height
#include <SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t byte;
typedef uint16_t word;
typedef struct { byte an; word dir; } sscar;
enum {c0=0,c1=1,c2=2,c3=3,c4=4,c_b_low=16,c_r_low=32};
extern int big;
extern byte *text_font;
extern sscar glyphs[256];
extern char cbs[65],cbn,cgs[65],cgn,crs[65],crn;
#define max_windows 16
/* Reduced from global.h:tventana; retains DIV's front-at-zero stack. */
struct tventana {
 int tipo,orden,primer_plano;
 const char *titulo;
 void (*paint_handler)(void),(*click_handler)(void),(*close_handler)(void);
 int x,y,an,al;
 Uint32 opened_at;
 int minimized,maximized,old_x,old_y,old_an,old_al,z;
 byte *ptr;
 int c_syntax; /* -1: filename, 0: plain, 1: C */
 size_t syntax_revision;
 byte *syntax_colors;
 Document *doc;
};
extern struct tventana ventana[max_windows];
#define v ventana[0]
void copy(int,int);
void move(int,int);
void divdelete(int);
void addwindow(void);
void wbox(byte*,int,int,byte,int,int,int,int);
void wbox_in_box(byte*,int,int,int,byte,int,int,int,int);
void wgra(byte*,int,int,byte,int,int,int,int);
void wrectangulo(byte*,int,int,byte,int,int,int,int);
int text_len(byte*);
void wwrite(byte*,int,int,int,int,int,byte*,byte);
void wwrite_in_box(byte*,int,int,int,int,int,int,byte*,byte);
void wtexn(byte*,int,byte*,int,int,byte,int,byte);
void wtexc(byte*,int,int,int,byte*,int,int,byte,int,byte);
int ui_init(void);
int ui_new(void);
void ui_request_close(void);
void ui_request_quit(void);
int ui_quit_ready(void);
int ui_confirmation(void);
void ui_close(void);
void ui_event(const SDL_Event*);
void ui_draw(byte*);
void ui_animations(int enabled);
int ui_set_resolution(int width,int height);
int ui_request_resolution(const char *text);
int ui_display_request(int *width,int *height,int *fullscreen);
void ui_error(const char *message);
int ui_open(const char *path);
void ui_draw_pointer(byte*,int,int);
void ui_destroy(void);
#endif
