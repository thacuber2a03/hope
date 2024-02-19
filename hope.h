#ifndef HOPE_H
#define HOPE_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>

#include "fe.h"

#define HOPE_VERSION "0.0.1"

#define FE_DATA_SIZE (1024 * 1024)
#define FE_ERROR_LEN 128
#define HOPE_ERROR_LEN 2048
#define HOPE_PATH_SIZE 0x7fff // reeeally unlikely to hit this limit
#define FILENAME_LEN 256
#define NORMAL_STR_LEN 256
#define TITLE_MAX_LEN 256

extern void load_api(fe_Context*);
extern fe_Object* hope_do_string(fe_Context*, const char*);

extern int wWidth;
extern int wHeight;
extern char wTitle[TITLE_MAX_LEN+1];
extern bool running;
extern char gameFolder[HOPE_PATH_SIZE];

extern ALLEGRO_COLOR cur_draw_col;
extern ALLEGRO_EVENT_QUEUE* event_queue;
extern ALLEGRO_DISPLAY* display;

extern ALLEGRO_KEYBOARD_STATE kState;
extern ALLEGRO_MOUSE_STATE    mState;

typedef enum { HOPE_SAMPLE, HOPE_SAMPLE_INSTANCE, HOPE_FONT, HOPE_IMAGE } hope_asset_type;
typedef struct { hope_asset_type type; void* value; } hope_asset;
extern hope_asset* current_font;
extern hope_asset* builtin_font;

#endif // HOPE_H
