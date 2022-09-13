#ifndef HOPE_H
#define HOPE_H

#include <SDL2/SDL.h>
#include "fe.h"

#define MAX_LEN_WINDOW_TITLE 64+1 // +1 to hold null char

#define WRONG_GLOBAL_TYPE_MSG "Wrong type passed to the %s global. Defaulting to %s...\n"
#define WINDOW_TITLE_GLOBAL_NAME "windowTitle"
#define WINDOW_DEFAULT_TITLE "hope"
#define WINDOW_SIZE_GLOBAL_NAME "windowSize"
#define WINDOW_DEFAULT_WIDTH 600
#define WINDOW_DEFAULT_HEIGHT 600

#define nextArgAsNumber(ctx, arg) (fe_tonumber((ctx), fe_nextarg((ctx), &(arg))))

extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern SDL_bool running;

#define FE_DATA_SIZE (1024*16)
extern void* ctx_data;
extern fe_Context* ctx;

struct APIFunc
{
	char* name;
	fe_Object* (*func)(fe_Context*, fe_Object*);
};

#define API_LEN 9
extern const struct APIFunc API[API_LEN];

#endif // HOPE_H
