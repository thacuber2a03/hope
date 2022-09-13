#include <math.h>

#include "hope.h"

static fe_Object* hope_color(fe_Context* ctx, fe_Object* arg)
{
    float r = nextArgAsNumber(ctx, arg);
    float g = nextArgAsNumber(ctx, arg);
    float b = nextArgAsNumber(ctx, arg);

    float a = 1;
    fe_Object* feObjA = fe_nextarg(ctx, &arg);
    if (fe_type(ctx, feObjA) != FE_TNIL) a = fe_tonumber(ctx, feObjA);

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    return fe_bool(ctx, SDL_FALSE);
}

static fe_Object* hope_cls(fe_Context* ctx, fe_Object* arg)
{
    SDL_RenderClear(renderer);
    return fe_bool(ctx, SDL_FALSE);
}

static fe_Object* hope_pset(fe_Context* ctx, fe_Object* arg)
{
    SDL_RenderDrawPoint(renderer, nextArgAsNumber(ctx, arg), nextArgAsNumber(ctx, arg));
    return fe_bool(ctx, SDL_FALSE);
}

static fe_Object* hope_rectfill(fe_Context* ctx, fe_Object* arg)
{
	float x = nextArgAsNumber(ctx, arg);
	float y = nextArgAsNumber(ctx, arg);
	float w = nextArgAsNumber(ctx, arg);
	float h = nextArgAsNumber(ctx, arg);
	SDL_Rect rect = {x, y, w, h};
	SDL_RenderFillRect(renderer, &rect);
	return fe_bool(ctx, SDL_FALSE);
}

fe_Object* hope_iskeypressed(fe_Context* ctx, fe_Object* arg)
{
    fe_Object* feObjKeyName = fe_nextarg(ctx, &arg);
    int keyNameSize = 20;
    char keyName[keyNameSize];
    fe_tostring(ctx, feObjKeyName, keyName, keyNameSize);

    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode keycode = SDL_GetKeyFromName(keyName);
    if (keycode == SDLK_UNKNOWN) fe_error(ctx, strcat("Unknown keyname", keyName));
    return fe_bool(ctx, state[SDL_GetScancodeFromKey(keycode)]);
}

static fe_Object* hope_quit(fe_Context* ctx, fe_Object* arg)
{
    running = SDL_FALSE;
    return fe_bool(ctx, SDL_FALSE);
}

// math functions

static fe_Object* hope_sqrt(fe_Context* ctx, fe_Object* arg) { return fe_number(ctx, sqrt(nextArgAsNumber(ctx, arg))); }
static fe_Object* hope_sin(fe_Context* ctx, fe_Object* arg) { return fe_number(ctx, sin(nextArgAsNumber(ctx, arg))); }
static fe_Object* hope_cos(fe_Context* ctx, fe_Object* arg) { return fe_number(ctx, cos(nextArgAsNumber(ctx, arg))); }

// API array

const struct APIFunc API[API_LEN] =
{
    {"cls",          hope_cls},
    {"color",        hope_color},
    {"pset",         hope_pset},
    {"rectfill",     hope_rectfill},
    {"iskeypressed", hope_iskeypressed},
    {"quit",         hope_quit},

    {"sqrt", hope_sqrt},
    {"sin",  hope_sin},
    {"cos",  hope_cos},
};
