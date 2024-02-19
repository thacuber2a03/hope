#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include "hope.h"

int wWidth = 600;
int wHeight = 600;
bool running = true;
char wTitle[TITLE_MAX_LEN+1] = "untitled";
char gameFolder[HOPE_PATH_SIZE];

static bool initedMain = false;

ALLEGRO_COLOR cur_draw_col = {255, 255, 255};
ALLEGRO_EVENT_QUEUE* event_queue = NULL;
hope_asset* builtin_font = NULL;
hope_asset* current_font = NULL;
ALLEGRO_DISPLAY* display = NULL;
ALLEGRO_KEYBOARD_STATE kState;
ALLEGRO_MOUSE_STATE mState;

void hope_error(char* errMsg, ...)
{
	char error[HOPE_ERROR_LEN];

	va_list args;
	va_start(args, errMsg);

	vsprintf(error, errMsg, args);

	va_end(args);

	printf("hope: %s\n", error);
	exit(EXIT_FAILURE);
}

char* fe_type_to_name(fe_Context* ctx, fe_Object* obj)
{
	switch (fe_type(ctx, obj))
	{
		case FE_TCFUNC:  return "cfunc";
		case FE_TFREE:   return "free";
		case FE_TFUNC:   return "func";
		case FE_TMACRO:  return "macro";
		case FE_TNIL:    return "nil";
		case FE_TNUMBER: return "number";
		case FE_TPAIR:   return "pair";
		case FE_TPRIM:   return "primitive";
		case FE_TPTR:    return "pointer";
		case FE_TSTRING: return "string";
		case FE_TSYMBOL: return "symbol";
		default: return "unknown";
	}
}

char* hope_get_asset_typename(int asset_type)
{
	switch (asset_type)
	{
		case HOPE_FONT:            return "hope:font";
		case HOPE_SAMPLE:          return "hope:sample";
		case HOPE_SAMPLE_INSTANCE: return "hope:sample-instance";
		case HOPE_IMAGE:           return "hope:image";
		default:                   hope_error("get_asset_typename: I'm, like, 90% sure this should be an error");
	}
}

static char read_str(fe_Context* ctx, void* udata)
{
	char** p = udata;
	if (!**p) return '\0';
	return *(*p)++;
}

fe_Object* hope_do_string(fe_Context* ctx, const char* str)
{
	char* p = (char*) str;
	fe_Object* obj = NULL;
	int gc = fe_savegc(ctx);
	for (;;)
	{
		fe_restoregc(ctx, gc);
		fe_Object* tmp = fe_read(ctx, read_str, &p);
		if (!tmp) break;
		obj = fe_eval(ctx, tmp);
	}
	if (obj) fe_pushgc(ctx, obj);
	return obj;
}

static bool hope_open_fe_file(fe_Context* ctx, char* filename, bool optional)
{
	FILE* fp = fopen(filename, "rb");
	bool file_exists = fp != NULL;
	if (file_exists)
	{
		while (true)
		{
			fe_Object* obj = fe_readfp(ctx, fp);
			if (!obj) break;
			fe_eval(ctx, obj);
		}
	}
	else if (!optional) hope_error("failed to load %s: %s", filename, strerror(errno));
	return file_exists;
}

static void hope_error_report(fe_Context* ctx, const char* err, fe_Object* cl)
{
	if (!display) display = al_create_display(wWidth, wHeight);
	al_set_window_title(display, "error!");

	char error[FE_ERROR_LEN];
	sprintf(error, "error: %s", err);
	fprintf(stderr, "%s\n", error);

	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_draw_text(builtin_font->value, al_map_rgb(255, 255, 255), 0, 0, 0, error);

	for (int i = 2; !fe_isnil(ctx, cl); cl = fe_cdr(ctx, cl), i++)
	{
		printf("=> ");
		char cltext[FE_ERROR_LEN];
		fe_tostring(ctx, fe_car(ctx, cl), cltext, FE_ERROR_LEN);
		fprintf(stderr, "%s\n", cltext);

		al_draw_text(builtin_font->value, al_map_rgb(255, 255, 255), 0, al_get_font_line_height(builtin_font->value)*i, 0, cltext);
	}

	al_flip_display();

	bool errScreen = true;
	while (errScreen)
	{
		ALLEGRO_EVENT event;
		if (!al_event_queue_is_empty(event_queue))
		{
			al_wait_for_event(event_queue, &event);
			switch (event.type)
			{
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				errScreen = false;
				break;
			case ALLEGRO_EVENT_KEY_DOWN:
				if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) errScreen = false;
				break;
			}
		}
	}

	al_destroy_font(builtin_font->value);
	free(builtin_font);

	if (event_queue) al_destroy_event_queue(event_queue);
	if (display) al_destroy_display(display);

	al_uninstall_audio();
	al_uninstall_keyboard();
	al_uninstall_mouse();

	exit(EXIT_FAILURE);
}

static fe_Object* hope_gc(fe_Context* ctx, fe_Object* arg)
{
	hope_asset* asset = fe_toptr(ctx, arg);
	printf("gotta collect a %s!\n", hope_get_asset_typename(asset->type));
	switch (asset->type)
	{
		case HOPE_FONT:
			if (!current_font == asset->value)
				al_destroy_font(asset->value);
			else puts("oh shit I can't do that, that's the current font");
			break;
		case HOPE_IMAGE:  al_destroy_bitmap(asset->value); break;
		case HOPE_SAMPLE: al_destroy_sample(asset->value); break;
		case HOPE_SAMPLE_INSTANCE:
			al_stop_sample_instance(asset->value);
			al_destroy_sample_instance(asset->value);
			break;
	}
	free(asset);
	return fe_bool(ctx, false);
}

int main(int argc, char* argv[])
{
	if (argc < 2) hope_error("please specify a folder");

	srand(time(NULL));

	al_init();
	al_init_primitives_addon();
	al_init_ttf_addon();
	al_init_font_addon();
	al_init_image_addon();
	al_init_acodec_addon();

	builtin_font = malloc(sizeof(hope_asset));
	builtin_font->type = HOPE_FONT;
	builtin_font->value = al_create_builtin_font();
	current_font = builtin_font;

	al_install_audio();
	al_init_acodec_addon();
	al_reserve_samples(10);
	//al_register_sample_loader(".wav", );

	al_install_keyboard();
	al_install_mouse();

	event_queue = al_create_event_queue();
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_mouse_event_source());

	strcpy(gameFolder, strcat(argv[1], "\\"));

	void* fe_data = malloc(FE_DATA_SIZE);
	if (!fe_data) hope_error("couldn't allocate data for fe");
	fe_Context* ctx = fe_open(fe_data, FE_DATA_SIZE);
	fe_handlers(ctx)->error = &hope_error_report;
	fe_handlers(ctx)->gc    = &hope_gc;

	{
		int gc = fe_savegc(ctx);
		char conf_loc[HOPE_PATH_SIZE];
		sprintf(conf_loc, "%s%s", gameFolder, "conf.fe");

		if (hope_open_fe_file(ctx, conf_loc, true))
		{
			fe_Object* fwidth  = fe_eval(ctx, fe_symbol(ctx, "width" ));
			fe_Object* fheight = fe_eval(ctx, fe_symbol(ctx, "height"));
			fe_Object* ftitle  = fe_eval(ctx, fe_symbol(ctx, "title" ));
			if (fe_type(ctx, fwidth ) == FE_TNUMBER) wWidth = fe_tonumber(ctx, fwidth);
			if (fe_type(ctx, fheight) == FE_TNUMBER) wHeight = fe_tonumber(ctx, fheight);
			if (fe_type(ctx, ftitle ) == FE_TSTRING) fe_tostring(ctx, ftitle, wTitle, TITLE_MAX_LEN);
		}
		//else printf("hope: conf.fe does not exist, defaulting...\n");

		hope_do_string(ctx, "(= width nil)");
		hope_do_string(ctx, "(= height nil)");
		hope_do_string(ctx, "(= title nil)");
	}

	al_set_app_name(wTitle);
	display = al_create_display(wWidth, wHeight);
	al_register_event_source(event_queue, al_get_display_event_source(display));

	load_api(ctx);
	initedMain = true;

	{
		char main_loc[HOPE_PATH_SIZE];
		sprintf(main_loc, "%s%s", gameFolder, "main.fe");
		hope_open_fe_file(ctx, main_loc, false);
		int gc = fe_savegc(ctx);
		fe_Object* update = fe_eval(ctx, fe_symbol(ctx, "hope:update"));
		// if (fe_isnil(ctx, update)) hope_error("function hope:update does not exist");
		if (!fe_isnil(ctx, update) && fe_type(ctx, update) != FE_TFUNC)
			hope_error("hope:update is not a function, why would you do that exactly?");
		fe_restoregc(ctx, gc);
	}

	hope_do_string(ctx, "(if hope:init (hope:init))");
	double last = 0, now = 0;
	while (running)
	{
		ALLEGRO_EVENT event;
		if (!al_event_queue_is_empty(event_queue))
		{
			al_wait_for_event(event_queue, &event);
			switch (event.type)
			{
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				running = false;
				break;
			}
		}

		now = al_get_time();
		double dt = now - last;
		last = now;

		al_get_keyboard_state(&kState);
		al_get_mouse_state(&mState);

		int gc = fe_savegc(ctx);
		fe_Object* update[] = { fe_symbol(ctx, "hope:update"), fe_number(ctx, dt) };
		if (!fe_isnil(ctx, fe_eval(ctx, fe_symbol(ctx, "hope:update")))) fe_eval(ctx, fe_list(ctx, update, 2));
		fe_restoregc(ctx, gc);

		al_flip_display();
	}

	hope_do_string(ctx, "(if hope:exit (hope:exit))");

	al_destroy_font(builtin_font->value);
	free(builtin_font);

	al_destroy_event_queue(event_queue);
	al_destroy_display(display);

	al_uninstall_mouse();
	al_uninstall_keyboard();
	al_uninstall_audio();

	fe_close(ctx);
	free(fe_data);

	return 0;
}
