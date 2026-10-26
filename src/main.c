/* New minimal Linux/SDL2 entry point. GPL-3.0. */
#include "ui.h"
#include "display.h"
#define W UI_WIDTH
#define H UI_HEIGHT
int main(int argc,char **argv) {
 int frames=0,limit=0,ok=0,demo=0,preview=0; const char *snapshot=NULL;
 for(int i=1;i<argc;i++) {
  if(!strcmp(argv[i],"--demo-dialog")) {demo=1;preview=1;}
  else if(!strcmp(argv[i],"--demo-help")) {demo=1;preview=3;}
  else if(!strcmp(argv[i],"--demo-menu")) {demo=1;preview=2;}
  else if(!strcmp(argv[i],"--smoke")) limit=3;
  else if(!strcmp(argv[i],"--demo")) demo=1;
  else if(!strcmp(argv[i],"--snapshot") && i+1<argc) snapshot=argv[++i];
  else { fprintf(stderr,"Usage: %s [--smoke] [--demo] [--snapshot file.bmp]\n",argv[0]); return 2; }
 }
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) { fprintf(stderr,"%s\n",SDL_GetError()); return 1; }
 SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
 SDL_Window *win=SDL_CreateWindow("DIVseed",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI);
 SDL_Renderer *renderer=win?SDL_CreateRenderer(win,-1,SDL_RENDERER_SOFTWARE):NULL;
 SDL_Texture *texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,W,H):NULL;
 SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);
 byte *indexed=malloc(W*H);
 if(!win || !renderer || !texture || !surface || !indexed) goto done;
 SDL_SetWindowMinimumSize(win,W,H);
 if(!display_configure(renderer,texture)) goto done;
 if(!ui_init()) { fprintf(stderr,"Cannot load bitmap fonts from assets beside executable\n"); goto done; }
 ui_animations(!limit);
 if(!ui_new()) goto done;
 if(demo) {
  document_paste(v.doc,"Notes / document one\n\nEach document keeps its own selection and history.\nUse File to open or safely save UTF-8 text.");
  if(!ui_new()) goto done;
  document_paste(v.doc,
   "DIVseed - alpha 0.1\n"
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
 if(preview==1) {SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_o;e.key.keysym.mod=KMOD_CTRL;ui_event(&e);}
 if(preview==2) {SDL_Event e={0};e.type=SDL_MOUSEMOTION;e.motion.x=20;e.motion.y=94;ui_event(&e);}
 if(preview==3) {SDL_Event e={0};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_F1;ui_event(&e);}
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
 /* Preserve the source palette and derive the obscured-window colors. */
 for(int i=0;i<64;i++) {Uint8 r,g,b;SDL_GetRGB(palette[i],surface->format,&r,&g,&b);palette[i+64]=SDL_MapRGB(surface->format,r*2/3,g*2/3,b*2/3);}
 int previous_cursor=SDL_ShowCursor(SDL_QUERY);
 int running=1;
 while(running) {
  SDL_Event e;
  while(SDL_PollEvent(&e)) {
   ui_event(&e);
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
