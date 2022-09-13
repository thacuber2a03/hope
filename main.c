#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hope.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_bool running = SDL_TRUE;

void* ctx_data = NULL;
fe_Context* ctx = NULL;

void configureWindow()
{
    // Window title
    SDL_SetWindowTitle(window, WINDOW_DEFAULT_TITLE);
    {
        fe_Object* feObjTitle = fe_eval(ctx, fe_symbol(ctx, WINDOW_TITLE_GLOBAL_NAME));
        if (!fe_isnil(ctx, feObjTitle))
        {
            if (fe_type(ctx, feObjTitle) == FE_TSTRING)
            {
                char windowTitle[MAX_LEN_WINDOW_TITLE];
                fe_tostring(ctx, feObjTitle, windowTitle, MAX_LEN_WINDOW_TITLE);
                SDL_SetWindowTitle(window, windowTitle);
            }
            else
                printf(WRONG_GLOBAL_TYPE_MSG, WINDOW_TITLE_GLOBAL_NAME, WINDOW_DEFAULT_TITLE);
        }
        else
            fe_set(ctx, fe_symbol(ctx, WINDOW_TITLE_GLOBAL_NAME), fe_string(ctx, WINDOW_DEFAULT_TITLE));
    }

    // Window size
    SDL_SetWindowSize(window, WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
    {
        fe_Object* feObjWinSize = fe_eval(ctx, fe_symbol(ctx, WINDOW_SIZE_GLOBAL_NAME));
        if (!fe_isnil(ctx, feObjWinSize))
        {
            if (fe_type(ctx, feObjWinSize) == FE_TPAIR)
            {
                int w = fe_tonumber(ctx, fe_car(ctx, feObjWinSize));
                int h = fe_tonumber(ctx, fe_cdr(ctx, feObjWinSize));
                SDL_SetWindowSize(window, w, h);
            }
            else
                printf("Wrong type passed to the %s global. Defaulting to (%i %i)...\n",
                       WINDOW_SIZE_GLOBAL_NAME,
                       WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);
        }
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

int main(int argc, char* argv[]) {
    if (argc > 1)
    {
        if (SDL_CreateWindowAndRenderer(600, 600, SDL_RENDERER_ACCELERATED, &window, &renderer) < 0)
        {
            printf("SDL couldn't initialize window and/or renderer. SDL_Error: %s\n", SDL_GetError());
            return EXIT_FAILURE;
        }

        // open main.fe and conf.fe
        char* foldername = malloc(strlen(argv[1])+1);
        strcpy(foldername, argv[1]);
        FILE* conffp = fopen(strcat(foldername, "/conf.fe"), "rb");
        if (!conffp) printf("Couldn't open conf.fe. Setting default configurations");

        strcpy(foldername, argv[1]);
        FILE* mainfp = fopen(strcat(foldername, "/main.fe"), "rb");
        if (!mainfp)
        {
            perror("Couldn't open main.fe");
            exit(EXIT_FAILURE);
        }
        free(foldername);

        // init fe
        ctx_data = malloc(FE_DATA_SIZE);
        if (!ctx_data)
        {
            perror("Couldn't initialize fe's data");
            exit(EXIT_FAILURE);
        }
        ctx = fe_open(ctx_data, FE_DATA_SIZE);

        for (int i=0; i<API_LEN; i++)
            fe_set(ctx,
                   fe_symbol(ctx, API[i].name),
                   fe_cfunc(ctx, API[i].func));

        int gc = fe_savegc(ctx);

        while (SDL_TRUE)
        {
            fe_Object* confobj = fe_readfp(ctx, conffp);
            if (!confobj) break;
            fe_eval(ctx, confobj);
            fe_restoregc(ctx, gc);
        }

        while (SDL_TRUE)
        {
            fe_Object* mainobj = fe_readfp(ctx, mainfp);
            if (!mainobj) break;
            fe_eval(ctx, mainobj);
            fe_restoregc(ctx, gc);
        }

        configureWindow(ctx);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        Uint32 lastTime = 0, deltaTime = 0;
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    running = SDL_FALSE;
                    break;
                }
            }

            Uint32 currentTime = SDL_GetTicks();
            deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            fe_Object* update[2] = {
                fe_symbol(ctx, "update"),
                fe_number(ctx, deltaTime)
            };
            fe_eval(ctx, fe_list(ctx, update, 2));
            fe_restoregc(ctx, gc);
            SDL_RenderPresent(renderer);
        }

        fe_close(ctx);
        free(ctx_data);

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
    else printf("Error: no folder passed.\nPlease pass the name of the folder your main.fe file is in.");

    return EXIT_SUCCESS;
}
