#include "ui.h"
#include "display.h"
/* Restore continuous aspect-preserving enlargement, with nearest sampling. */
int display_configure(SDL_Renderer *renderer,SDL_Texture *texture) {
 if(SDL_SetTextureScaleMode(texture,SDL_ScaleModeNearest)) return 0;
 if(SDL_RenderSetLogicalSize(renderer,UI_WIDTH,UI_HEIGHT)) return 0;
 if(SDL_RenderSetIntegerScale(renderer,SDL_FALSE)) return 0;
 return 1;
}

/* Desktop fullscreen keeps the monitor mode and SDL restores window geometry. */
int display_toggle_fullscreen(SDL_Window *window) {
 Uint32 mode=SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN_DESKTOP;
 return SDL_SetWindowFullscreen(window,mode?0:SDL_WINDOW_FULLSCREEN_DESKTOP)==0;
}
