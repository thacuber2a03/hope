#include <string.h>
#include <stdarg.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265359
#endif // M_PI

#include "hope.h"

#define api_func(x) static fe_Object* hope_api_##x(fe_Context* ctx, fe_Object* arg)

#define next_arg_as_number() fe_tonumber(ctx, fe_nextarg(ctx, &arg))
#define next_arg_as_integer() ((int) next_arg_as_number())
#define next_arg_as_asset_pointer() ((hope_asset*) fe_toptr(ctx, fe_nextarg(ctx, &arg)))
#define next_arg_as_string(var, size) char var[(size) + 1]; fe_tostring(ctx, fe_nextarg(ctx, &arg), var, (size)+1)

#define assert_asset_type(ast, typ) \
	if (ast->type != typ) fe_ferror(ctx, "expected a %s, got a %s instead", hope_get_asset_typename(typ), hope_get_asset_typename(ast->type))

#define len_full_path(relfplen) (strlen(gameFolder)+(relfplen)+1)
#define generate_path_vars() \
	next_arg_as_string(filename, FILENAME_LEN); \
	char filepath[len_full_path(strlen(filename))]; \
	get_game_folder_path(filename, filepath)

#define nil fe_bool(ctx, false)

static void fe_ferror(fe_Context* ctx, char* format, ...)
{
	char err[FE_ERROR_LEN];

	va_list args;
	va_start(args, format);

	vsprintf(err, format, args);

	va_end(args);

	fe_error(ctx, err);
}

static void get_game_folder_path(char* rel_path, char* ret_path)
{
	char abs_path[strlen(gameFolder)+strlen(rel_path)+1];
	sprintf(abs_path, "%s%s", gameFolder, rel_path);
	strcpy(ret_path, abs_path);
}

/////////////////////// graphics ///////////////////////

api_func(clear)
{
	al_clear_to_color(cur_draw_col);
	return nil;
}

api_func(set_color)
{
	fe_Number c[4] = {0, 0, 0, 1};

	fe_Object* next = fe_nextarg(ctx, &arg);
	if (fe_type(ctx, next) == FE_TNUMBER)
	{
		c[0] = fe_tonumber(ctx, next);
		c[1] = next_arg_as_number();
		c[2] = next_arg_as_number();
		if (!fe_isnil(ctx, arg)) c[3] = next_arg_as_number();
	}
	else if (fe_type(ctx, next) == FE_TPAIR)
	{
		fe_Object* obj = next;
		for (int i=0; !fe_isnil(ctx, obj); obj = fe_cdr(ctx, obj), i++)
			c[i] = fe_tonumber(ctx, fe_car(ctx, obj));
	}

	cur_draw_col = al_map_rgba_f(c[0], c[1], c[2], c[3]);
	return nil;
}

api_func(rect)
{
	next_arg_as_string(mode, 4);

	fe_Number x =   next_arg_as_number(),
	          y =   next_arg_as_number(),
	          w = x+next_arg_as_number(),
	          h = y+next_arg_as_number();

	if      (strcmp(mode, "line") == 0) al_draw_rectangle(x, y, w, h, cur_draw_col, 1);
	else if (strcmp(mode, "fill") == 0) al_draw_filled_rectangle(x, y, w, h, cur_draw_col);
	else fe_ferror(ctx, "Unknown draw mode: %s", mode);

	return nil;
}

api_func(circ)
{
	next_arg_as_string(mode, 4);

	fe_Number x = next_arg_as_number(),
	          y = next_arg_as_number(),
	          r = next_arg_as_number();

	if       (strcmp(mode, "line") == 0) al_draw_circle(x, y, r, cur_draw_col, 1);
	else if (strcmp(mode, "fill") == 0) al_draw_filled_circle(x, y, r, cur_draw_col);
	else fe_ferror(ctx, "Unknown draw mode '%s'", mode);

	return nil;
}

api_func(line)
{
	fe_Number x1 = next_arg_as_number(),
	          y1 = next_arg_as_number(),
	          x2 = next_arg_as_number(),
	          y2 = next_arg_as_number(),
	          thk = 1;

	if (!fe_isnil(ctx, arg)) thk = next_arg_as_number();

	al_draw_line(x1, y1, x2, y2, cur_draw_col, thk);

	return nil;
}

api_func(text)
{
	next_arg_as_string(text, 128); // temporary, you can't just let people only be able to write 128 characters

	fe_Number x = next_arg_as_number(),
	          y = next_arg_as_number();

	hope_asset* font = current_font;
	if (!fe_isnil(ctx, arg))
	{
		font = next_arg_as_asset_pointer();
		assert_asset_type(font, HOPE_FONT);
	}

	al_draw_text(font->value, cur_draw_col, x, y, 0, text);
	return nil;
}

api_func(triangle)
{
	next_arg_as_string(mode, 4);

	fe_Number x1 = next_arg_as_number(), y1 = next_arg_as_number(),
	          x2 = next_arg_as_number(), y2 = next_arg_as_number(),
	          x3 = next_arg_as_number(), y3 = next_arg_as_number();

	if      (strcmp(mode, "line") == 0) al_draw_triangle       (x1, y1, x2, y2, x3, y3, cur_draw_col, 1);
	else if (strcmp(mode, "fill") == 0) al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, cur_draw_col);
	else fe_ferror(ctx, "Unknown draw mode: %s", mode);

	return nil;
}

//////////////////////// input /////////////////////////

api_func(key_down)
{
	char keyname[FILENAME_LEN];
	if (!fe_isnil(ctx, arg)) fe_tostring(ctx, fe_nextarg(ctx, &arg), keyname, sizeof keyname);

	for (int i=0; i<ALLEGRO_KEY_MAX; i++)
	{
		if (al_key_down(&kState, i+1))
		{
			if (keyname[0] == '\0') return fe_bool(ctx, true);
			else
			{
				char curKeyName[strlen(al_keycode_to_name(i+1))];
				strcpy(curKeyName, al_keycode_to_name(i+1));
				if (strcasecmp(keyname, curKeyName) == 0) return fe_bool(ctx, true);
			}
		}
	}

	return fe_bool(ctx, false);
}

api_func(get_mouse_pos)
{
	int x = mState.x; int y = mState.y;
	return fe_cons(ctx, fe_number(ctx, x), fe_number(ctx, y));
}

api_func(mouse_button_down)
{
	next_arg_as_string(btn, 0xf);

	if (strcmp(btn, "right" )) return fe_bool(ctx, mState.buttons & 1);
	if (strcmp(btn, "left"  )) return fe_bool(ctx, mState.buttons & 2);
	if (strcmp(btn, "middle")) return fe_bool(ctx, mState.buttons & 4);

	fe_ferror(ctx, "unknown mouse button: %s", btn);
	return nil;
}

//////////////////////// images ////////////////////////

api_func(load_image)
{
	generate_path_vars();

	ALLEGRO_BITMAP* _image = al_load_bitmap(filepath);
	if (_image == NULL) fe_ferror(ctx, "couldn't load image: %s", strerror(al_get_errno()));
	hope_asset* image = malloc(sizeof(hope_asset));
	image->type = HOPE_IMAGE;
	image->value = _image;

	return fe_ptr(ctx, image);
}

api_func(get_image_width)
{
	hope_asset* image = next_arg_as_asset_pointer();
	assert_asset_type(image, HOPE_IMAGE);
	return fe_number(ctx, al_get_bitmap_width(image->value));
}

api_func(get_image_height)
{
	hope_asset* image = next_arg_as_asset_pointer();
	assert_asset_type(image, HOPE_IMAGE);
	return fe_number(ctx, al_get_bitmap_height(image->value));
}

api_func(draw_image)
{
	hope_asset* image = next_arg_as_asset_pointer();
	assert_asset_type(image, HOPE_IMAGE);

	ALLEGRO_BITMAP* bitmap = image->value;
	int bWidth  = al_get_bitmap_width(bitmap);
	int bHeight = al_get_bitmap_height(bitmap);

	// just draw the image. cool.
	fe_Number x = next_arg_as_number(),
	          y = next_arg_as_number();

	if (fe_isnil(ctx, arg)) al_draw_tinted_bitmap(bitmap, cur_draw_col, x, y, 0);
	else
	{
		// scaling? okaaayy...
		fe_Number w = next_arg_as_number(),
		          h = next_arg_as_number();

		if (fe_isnil(ctx, arg)) al_draw_tinted_scaled_bitmap(bitmap, cur_draw_col, 0, 0, bWidth, bHeight, x, y, w, h, 0);
		else
		{
			// oh lord, rotating too??
			fe_Number r = next_arg_as_number(), cx = 0.5, cy = 0.5;

			if (!fe_isnil(ctx, arg))
			{
				cx = next_arg_as_number();
				cy = next_arg_as_number();
			}

			float xScale = w/bWidth;
			float yScale = h/bHeight;
			cy *= h; cx *= w;

			al_draw_tinted_scaled_rotated_bitmap(
				bitmap, cur_draw_col, cx/xScale, cy/yScale,
				x+cx, y+cy, xScale, yScale,
				r, 0
			);
		}
	}
	return nil;
}

api_func(unload_image)
{
	hope_asset* image = next_arg_as_asset_pointer();
	assert_asset_type(image, HOPE_IMAGE);
	al_destroy_bitmap(image->value);
	free(image);
	return nil;
}

//////////////////////// sounds ////////////////////////

api_func(load_sample)
{
	char filename[FILENAME_LEN];
	fe_tostring(ctx, fe_nextarg(ctx, &arg), filename, sizeof filename);
	char filepath[len_full_path(strlen(filename))];
	get_game_folder_path(filename, filepath);

	ALLEGRO_SAMPLE* _sample = al_load_sample(filepath);
	if (_sample == NULL)
	{
		fe_ferror(ctx, "couldn't load sound: %s", strerror(al_get_errno()) );
		return nil;
	}

	ALLEGRO_SAMPLE_INSTANCE* _sampleInstance = al_create_sample_instance(_sample);
	al_attach_sample_instance_to_mixer(_sampleInstance, al_get_default_mixer());
	hope_asset* sample = malloc(sizeof(hope_asset));
	sample->type = HOPE_SAMPLE;
	sample->value = _sample;

	return fe_ptr(ctx, sample);
}

api_func(make_sample_instance)
{
	hope_asset* sample = next_arg_as_asset_pointer();
	assert_asset_type(sample, HOPE_SAMPLE);

	ALLEGRO_SAMPLE_INSTANCE* _instance = al_create_sample_instance(sample->value);
	al_attach_sample_instance_to_mixer(_instance, al_get_default_mixer());

	hope_asset* instance = malloc(sizeof(instance));
	instance->type = HOPE_SAMPLE_INSTANCE;
	instance->value = _instance;
	return fe_ptr(ctx, instance);
}

api_func(play_sample_instance)
{
	hope_asset* instance = next_arg_as_asset_pointer();
	assert_asset_type(instance, HOPE_SAMPLE_INSTANCE);
	al_play_sample_instance(instance->value);
	return nil;
}

api_func(unload_sample_instance)
{
	hope_asset* instance = next_arg_as_asset_pointer();
	assert_asset_type(instance, HOPE_SAMPLE_INSTANCE);
	al_destroy_sample_instance(instance->value);
	free(instance);
	return nil;
}

api_func(unload_sample)
{
	hope_asset* sample = next_arg_as_asset_pointer();
	assert_asset_type(sample, HOPE_SAMPLE);
	al_destroy_sample(sample->value);
	free(sample);
	return nil;
}

//////////////////////// fonts /////////////////////////

api_func(get_string_len)
{
	next_arg_as_string(str, NORMAL_STR_LEN);
	return fe_number(ctx, strlen(str));
}

api_func(load_font)
{
	generate_path_vars();

	int size = next_arg_as_number();
	ALLEGRO_FONT* _font = al_load_font(filepath, size, 0);
	if (_font == NULL) fe_ferror(ctx, "couldn't load font: %s", strerror(al_get_errno()));

	hope_asset* font = malloc(sizeof(hope_asset));
	font->type = HOPE_FONT;
	font->value = _font;
	return fe_ptr(ctx, font);
}

api_func(set_font)         { current_font = next_arg_as_asset_pointer(); return nil; }
api_func(get_font)         { return fe_ptr(ctx, current_font); }
api_func(get_builtin_font) { return fe_ptr(ctx, builtin_font); }

api_func(get_font_width)
{
	next_arg_as_string(text, NORMAL_STR_LEN);

	hope_asset* font = current_font;
	if (!fe_isnil(ctx, arg))
	{
		font = next_arg_as_asset_pointer();
		assert_asset_type(font, HOPE_FONT);
	}
	return fe_number(ctx, al_get_text_width(font->value, text));
}

api_func(get_font_height)
{
	hope_asset* font = current_font;
	if (!fe_isnil(ctx, arg))
	{
		font = next_arg_as_asset_pointer();
		assert_asset_type(font, HOPE_FONT);
	}
	return fe_number(ctx, al_get_font_line_height(font->value));
}

api_func(unload_font)
{
	hope_asset* font = next_arg_as_asset_pointer();
	if (font == builtin_font) fe_ferror(ctx, "you can't unload the built-in font!");
	assert_asset_type(font, HOPE_FONT);
	al_destroy_font(font->value);
	free(font);
	return nil;
}

/////////////// other asset related things /////////////

api_func(get_asset_type)
{
	hope_asset* asset = next_arg_as_asset_pointer();
	return fe_string(ctx, hope_get_asset_typename(asset));
}

///////////////////////// math /////////////////////////

api_func(floor) { return fe_number(ctx, floor(next_arg_as_number())); }
api_func(ceil)  { return fe_number(ctx, ceil(next_arg_as_number())); }
api_func(pow)   { return fe_number(ctx, pow(next_arg_as_number(), next_arg_as_number())); }
api_func(sqrt)  { return fe_number(ctx, sqrt(next_arg_as_number())); }
api_func(mod)   { return fe_number(ctx, fmod(next_arg_as_number(), next_arg_as_number())); }

api_func(sin)   { return fe_number(ctx, sin(next_arg_as_number())); }
api_func(cos)   { return fe_number(ctx, cos(next_arg_as_number())); }
api_func(tan)   { return fe_number(ctx, tan(next_arg_as_number())); }
api_func(atan2) { return fe_number(ctx, atan2(next_arg_as_number(), next_arg_as_number())); }

// I'm not a dumbass
api_func(band) { return fe_number(ctx, next_arg_as_integer() & next_arg_as_integer() ); }
api_func(bor)  { return fe_number(ctx, next_arg_as_integer() | next_arg_as_integer() ); }
api_func(bnot) { return fe_number(ctx, ~next_arg_as_integer() ); }
api_func(bxor) { return fe_number(ctx, next_arg_as_integer() ^ next_arg_as_integer() ); }

api_func(brsh) { return fe_number(ctx, next_arg_as_integer() >> next_arg_as_integer() ); }
api_func(blsh) { return fe_number(ctx, next_arg_as_integer() << next_arg_as_integer() ); }

//////////////////////// system ////////////////////////

api_func(do_file)
{
	generate_path_vars();

	fe_Object* obj = NULL;
	FILE* fp = fopen(filepath, "rb");
	int gc = fe_savegc(ctx);
	for (;;)
	{
		fe_restoregc(ctx, gc);
		fe_Object* tmp = fe_readfp(ctx, fp);
		if (!tmp) break;
		obj = fe_eval(ctx, tmp);
	}
	if (obj) fe_pushgc(ctx, obj);
	return obj;
}

static void update_display() { al_resize_display(display, wWidth, wHeight); }

api_func(set_width)  { wWidth  = next_arg_as_integer(); update_display(); return nil; }
api_func(set_height) { wHeight = next_arg_as_integer(); update_display(); return nil; }
api_func(set_title)
{
	next_arg_as_string(title, TITLE_MAX_LEN);
	strcpy(wTitle, title);
	update_display();
	return nil;
}

api_func(get_width ) { return fe_number(ctx, wWidth);  }
api_func(get_height) { return fe_number(ctx, wHeight); }
api_func(get_title ) { return fe_string(ctx, wTitle);  }

api_func(rand) { return fe_number(ctx, (fe_Number) rand() / RAND_MAX); }

api_func(type) { return fe_string(ctx, fe_type_to_name(ctx, fe_nextarg(ctx, &arg))); }

api_func(error)
{
	next_arg_as_string(err, FE_ERROR_LEN);
	fe_error(ctx, err);
	return nil;
}

api_func(quit) { running = false; return nil; }

////////////////////////////////////////////////////////

static struct { char* name; fe_CFunc func; } api[] =
{
	{ "hope:clear",     hope_api_clear     },
	{ "hope:set-color", hope_api_set_color },
	{ "hope:rect",      hope_api_rect      },
	{ "hope:circ",      hope_api_circ      },
	{ "hope:triangle",  hope_api_triangle  },
	{ "hope:line",      hope_api_line      },
	{ "hope:text",      hope_api_text      },

	{ "hope:load-image",       hope_api_load_image       },
	{ "hope:get-image-width",  hope_api_get_image_width  },
	{ "hope:get-image-height", hope_api_get_image_height },
	{ "hope:draw-image",       hope_api_draw_image       },
	{ "hope:unload-image",     hope_api_unload_image     },

	{ "hope:load-sample",            hope_api_load_sample            },
	{ "hope:make-sample-instance",   hope_api_make_sample_instance   },
	{ "hope:play-sample-instance",   hope_api_play_sample_instance   },
	{ "hope:unload-sample-instance", hope_api_unload_sample_instance },
	{ "hope:unload-sample",          hope_api_unload_sample          },

	{ "hope:load-font",        hope_api_load_font        },
	{ "hope:unload-font",      hope_api_unload_font      },
	{ "hope:set-font",         hope_api_set_font         },
	{ "hope:get-font",         hope_api_get_font         },
	{ "hope:get-builtin-font", hope_api_get_builtin_font },
	{ "hope:get-font-width",   hope_api_get_font_width   },
	{ "hope:get-font-height",  hope_api_get_font_height  },

	{ "hope:get-asset-type", hope_api_get_asset_type },

	{ "hope:key-down",          hope_api_key_down          },
	{ "hope:get-mouse-pos",     hope_api_get_mouse_pos     },
	{ "hope:mouse-button-down", hope_api_mouse_button_down },

	{ "floor", hope_api_floor },
	{ "ceil",  hope_api_ceil  },
	{ "sqrt",  hope_api_sqrt  },
	{ "pow",   hope_api_pow   },
	{ "%",     hope_api_mod   },
	{ "sin",   hope_api_sin   },
	{ "cos",   hope_api_cos   },
	{ "tan",   hope_api_tan   },
	{ "atan2", hope_api_atan2 },

	{ "&",  hope_api_band },
	{ "|",  hope_api_bor  },
	{ "~",  hope_api_bnot },
	{ "^",  hope_api_bxor },
	{ ">>", hope_api_brsh },
	{ "<<", hope_api_blsh },

	{ "require", hope_api_do_file        },
	{ "#",       hope_api_get_string_len },

	{ "hope:set-width",  hope_api_set_width  },
	{ "hope:set-height", hope_api_set_height },
	{ "hope:set-title",  hope_api_set_title  },
	{ "hope:get-width",  hope_api_get_width  },
	{ "hope:get-height", hope_api_get_height },
	{ "hope:get-title",  hope_api_get_title  },

	{ "hope:rand",  hope_api_rand  },
	{ "hope:error", hope_api_error },
	{ "hope:type",  hope_api_type  },
	{ "hope:quit",  hope_api_quit  },
};

#define API_ENTRIES (sizeof(api) / sizeof(api[0]))

static void load_literals(fe_Context* ctx)
{
	fe_set(ctx, fe_symbol(ctx, "hope:pi" ), fe_number(ctx, M_PI));
	fe_set(ctx, fe_symbol(ctx, "hope:tau"), fe_number(ctx, M_PI*2));
	fe_set(ctx, fe_symbol(ctx, "hope:version"), fe_string(ctx, HOPE_VERSION));
	fe_set(ctx, fe_symbol(ctx, "hope:fe-version"), fe_string(ctx, FE_VERSION));
}

char std[] = "\
	(= macro (mac (sym params . body) (list '= sym (cons 'mac (cons params body)) ) )) \
	(macro func (sym params . body) (list '= sym (cons 'fn (cons params body)) ) ) \
	\
	(macro >  (a b) (list '<  b a) ) \
	(macro >= (a b) (list '<= b a) ) \
	\
	(macro += (a b) (list '= a (list '+ a b)) ) \
	(macro -= (a b) (list '= a (list '- a b)) ) \
	(macro /= (a b) (list '= a (list '* a b)) ) \
	(macro *= (a b) (list '= a (list '/ a b)) ) \
	\
	(macro neg (x) (list '- 0 x) ) \
	(macro inc (x) (list '= x (list '+ x '1)) ) \
	(macro dec (x) (list '= x (list '- x '1)) ) \
	(macro ++ (x) (list 'inc x))\
	(macro -- (x) (list 'dec x))\
	\
	(macro hope:abs (x)  (list 'if (list '< x 0) (list 'neg x) x) ) \
	(macro hope:min (a b) (list 'if (list '< a b) a b) ) \
	(macro hope:max (a b) (list 'if (list '> a b) a b) ) \
	(func  hope:mid (v mn mx) (hope:max mn (hope:min v mx))) \
	(func  hope:random (a b) (+ a (* (- b a) (hope:rand)))) \
	(func  hope:distSqr (x1 y1 x2 y2) \
		(let dx (- x2 x1)) \
		(let dy (- y2 y1)) \
		(+ (* dx dx) (* dy dy)) \
	) \
	\
	(func hope:dist (x1 y1 x2 y2) (sqrt (hope:distSqr x1 y1 x2 y2)) ) \
	\
	(macro when (cond . body) (list 'if cond (cons 'do body))) \
	(macro unless (cond . body) (list 'if (list 'not cond) (cons 'do body))) \
	(macro return (x) x) \
	(macro for (var lst . body) \
		(list 'do \
			(list 'let 'l lst)\
			(list 'while 'l \
				(list 'let var '(car l)) \
				(cons 'do body)\
				'(= l (cdr l)) \
			) \
		) \
	) \
	(func range (n) \
		 (let res nil) \
		 (while (> n 0) \
			(= res (cons n res)) \
			(dec n) \
		 ) \
		 res \
	) \
";

void load_api(fe_Context* ctx)
{
	int gc = fe_savegc(ctx);
	for (int i = 0; i < API_ENTRIES; i++)
	{
		fe_set(ctx,
			fe_symbol(ctx, api[i].name),
			fe_cfunc (ctx, api[i].func)
		);
		fe_restoregc(ctx, gc);
	}
	load_literals(ctx);
	fe_restoregc(ctx, gc);
	hope_do_string(ctx, std);
	fe_restoregc(ctx, gc);
}
