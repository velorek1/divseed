/* New minimal Linux/SDL2 entry point. GPL-3.0. */
#include "ui.h"
#include "display.h"
#define W UI_WIDTH
#define H UI_HEIGHT
int main(int argc,char **argv) {
 int frames=0,limit=0,ok=0,demo=0,preview=0; const char *snapshot=NULL,*files[16];int file_count=0,end_options=0;
 for(int i=1;i<argc;i++) {
  if(end_options || argv[i][0]!='-' || !strcmp(argv[i],"-")) {
   if(file_count==16) {fprintf(stderr,"At most 16 files can be opened.\n");return 2;}files[file_count++]=argv[i];
  }
  else if(!strcmp(argv[i],"--")) end_options=1;
  else if(!strcmp(argv[i],"--help")) {puts("Usage: divseed [--] [file ...]\nAlt+Enter: fullscreen. Display menu: resolutions.");return 0;}
  else if(!strcmp(argv[i],"--demo-reference")) {demo=1;preview=4;}
  else if(!strcmp(argv[i],"--demo-dialog")) {demo=1;preview=1;}
  else if(!strcmp(argv[i],"--demo-help")) {demo=1;preview=3;}
  else if(!strcmp(argv[i],"--demo-menu")) {demo=1;preview=2;}
  else if(!strcmp(argv[i],"--smoke")) limit=3;
  else if(!strcmp(argv[i],"--demo")) demo=1;
  else if(!strcmp(argv[i],"--snapshot") && i+1<argc) snapshot=argv[++i];
  else { fprintf(stderr,"Usage: %s [--smoke] [--demo] [--snapshot file.bmp] [--] [file ...]\n",argv[0]); return 2; }
 }
 if(file_count && demo) {fprintf(stderr,"Choose either files or demo mode.\n");return 2;}
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) { fprintf(stderr,"%s\n",SDL_GetError()); return 1; }
 SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
 SDL_Window *win=SDL_CreateWindow("DIVseed",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
 SDL_Renderer *renderer=win?SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE):NULL;
 SDL_Texture *texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,W,H):NULL;
 SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
 byte *indexed=malloc(W*H);
 if(!win || !renderer || !texture || !surface || !indexed) goto done;
 SDL_SetWindowMinimumSize(win,800,600);
 if(!display_configure(renderer,texture)) goto done;
 if(!ui_init()) { fprintf(stderr,"Cannot load bitmap fonts from assets beside executable\n"); goto done; }
 ui_animations(!limit);
 if(file_count) {for(int i=0;i<file_count;i++) if(!ui_open(files[i])) goto done;}
 else if(!ui_new()) goto done;
 if(demo) {
  document_paste(v.doc,"Notes / document one\n\nEach document keeps its own selection and history.\nUse File to open or safely save UTF-8 text.");
  if(!ui_new()) goto done;
  document_paste(v.doc,
   "DIVseed - alpha 0.4\n"
   "\n"
   "A familiar workspace, built from DIV's original graphics.\n"
   "\n"
   "Editing\n"
   "  Click to place the cursor; drag to select text.\n"
   "  Hold Shift with arrows, Home or End to extend a selection.\n"
   "  Ctrl+A selects the complete document.\n"
   "\n"
   "Clipboard and history\n"
   "  Ctrl+C / Ctrl+X / Ctrl+V: copy, cut and paste.\n"
   "  Ctrl+Z: undo.  Ctrl+Y or Ctrl+Shift+Z: redo.\n"
   "  Each window keeps its own undo and redo history.\n"
   "\n"
   "Original resources\n"
   "  SYS08X11.BIN - 8 x 11 editor bitmap font\n"
   "  grande.fon   - original interface lettering\n"
   "  GRAF_G.DIV   - original close and scrollbar icons\n"
   "\n"
   "Ctrl+O: open | Ctrl+S: save | Ctrl+Shift+S: Save As\n"
   "UTF-8: español, año, pingüino, € Ω Ж → ☺\n");
  document_point(v.doc,3,3,0);document_point(v.doc,3,21,1);
 }
 if(demo && !preview) {
  document_select_all(v.doc);
  document_paste(v.doc,"/* DIVseed: C colors at 960 x 600 */\n#include <stdio.h>\n\n#define ANSWER 42\n\nint main(void) {\n    const char *message = \"Hola, Raúl!\";\n    // Original 8 x 11 bitmap font\n    printf(\"%s: %d\\n\", message, ANSWER);\n    return 0;\n}\n");
  v.c_syntax=1;document_point(v.doc,1,1,0);
 }
 if(preview==1) {SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_o;e.key.keysym.mod=KMOD_CTRL;ui_event(&e);}
 if(preview==2) {SDL_Event e={0};e.type=SDL_MOUSEMOTION;e.motion.x=20;e.motion.y=94;ui_event(&e);}
 if(preview==3) {SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_F1;ui_event(&e);}
 if(preview==4) {SDL_Event e={0};e.type=SDL_MOUSEBUTTONDOWN;e.button.button=1;e.button.x=170;e.button.y=15;ui_event(&e);e.type=SDL_MOUSEBUTTONUP;ui_event(&e);}
 SDL_StartTextInput();
 uint32_t palette[256];
 for(int i=0;i<256;i++) palette[i]=SDL_MapRGB(surface->format,i,i,i);
 const byte rgb[][3]={{15,19,25},{43,49,57},{98,108,117},{174,186,193},{241,238,213},{25,31,38},{35,55,63},{44,66,72}};
 for(int i=0;i<8;i++) palette[i]=SDL_MapRGB(surface->format,rgb[i][0],rgb[i][1],rgb[i][2]);
 for(int i=0;i<8;i++) { palette[16+i]=SDL_MapRGB(surface->format,22+i*5,40+i*9,76+i*14); palette[32+i]=SDL_MapRGB(surface->format,70+i*17,25,32); }
 palette[8]=SDL_MapRGB(surface->format,112,112,112);
 palette[9]=SDL_MapRGB(surface->format,204,204,204);
 palette[10]=SDL_MapRGB(surface->format,255,255,255);
 palette[12]=SDL_MapRGB(surface->format,72,72,72);
 palette[11]=SDL_MapRGB(surface->format,0,0,0);
 const byte syntax_rgb[][3]={{120,192,255},{133,174,130},{233,190,123},{202,157,238},{224,143,174}};
 for(int i=0;i<5;i++) palette[48+i]=SDL_MapRGB(surface->format,syntax_rgb[i][0],syntax_rgb[i][1],syntax_rgb[i][2]);
 /* Preserve the source palette and derive the obscured-window colors. */
 for(int i=0;i<64;i++) {Uint8 r,g,b;SDL_GetRGB(palette[i],surface->format,&r,&g,&b);palette[i+64]=SDL_MapRGB(surface->format,r*2/3,g*2/3,b*2/3);}
 int previous_cursor=SDL_ShowCursor(SDL_QUERY);
 int running=1;
 while(running) {
  SDL_Event e;
  while(SDL_PollEvent(&e)) {
   ui_event(&e);
  }
  int width,height,fullscreen;
  if(ui_display_request(&width,&height,&fullscreen)) {
   if(fullscreen && !display_toggle_fullscreen(win)) ui_error(SDL_GetError());
   if(width && (width!=W || height!=H)) {
    SDL_Surface *next_surface=SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_ARGB8888);
    byte *next_indexed=malloc((size_t)width*height);
    SDL_Texture *next_texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,width,height);
    if(!next_surface || !next_indexed || !next_texture || !ui_set_resolution(width,height)) {
     SDL_FreeSurface(next_surface);free(next_indexed);SDL_DestroyTexture(next_texture);ui_error("Cannot change resolution: allocation failed.");
    } else {
     SDL_FreeSurface(surface);free(indexed);SDL_DestroyTexture(texture);
     surface=next_surface;indexed=next_indexed;texture=next_texture;
     if(!display_configure(renderer,texture)) goto done;
     if(!(SDL_GetWindowFlags(win)&SDL_WINDOW_FULLSCREEN_DESKTOP)) SDL_SetWindowSize(win,W,H);
    }
   }
  }
  if(ui_quit_ready()) running=0;
  ui_draw(indexed);
  if(!limit) {
   int mx,my;float lx,ly;SDL_GetMouseState(&mx,&my);SDL_RenderWindowToLogical(renderer,mx,my,&lx,&ly);
   int inside=SDL_GetMouseFocus()==win && lx>=0 && ly>=0 && lx<W && ly<H;
   SDL_ShowCursor(inside?SDL_DISABLE:SDL_ENABLE);
   if(inside) ui_draw_pointer(indexed,(int)lx,(int)ly);
  }
  for(int y=0;y<H;y++) {
   uint32_t *row=(uint32_t*)((byte*)surface->pixels+y*surface->pitch);
   for(int x=0;x<W;x++) row[x]=palette[indexed[y*W+x]];
  }
  if(SDL_UpdateTexture(texture,NULL,surface->pixels,surface->pitch) || SDL_RenderClear(renderer) || SDL_RenderCopy(renderer,texture,NULL,NULL)) goto done;
  SDL_RenderPresent(renderer);
  if(limit && ++frames>=limit) running=0;
  SDL_Delay(16);
 }
 SDL_ShowCursor(previous_cursor);
 if(snapshot && SDL_SaveBMP(surface,snapshot)) goto done;
 ok=1;
done:
 if(!ok) fprintf(stderr,"Startup/render failed: %s\n",SDL_GetError());
 ui_destroy(); free(indexed); SDL_FreeSurface(surface);
 SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(win); SDL_Quit();
 return ok?0:1;
}
