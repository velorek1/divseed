#include "ui.h"
#include "display.h"
/* Restore continuous aspect-preserving enlargement, with nearest sampling. */
int display_configure(SDL_Renderer *renderer,SDL_Texture *texture) {
 if(SDL_SetTextureScaleMode(texture,SDL_ScaleModeNearest)) return 0;
 if(SDL_RenderSetLogicalSize(renderer,UI_WIDTH,UI_HEIGHT)) return 0;
 if(SDL_RenderSetIntegerScale(renderer,SDL_FALSE)) return 0;
 return 1;
}
