#include "display.h"
#include "Communication.h"

#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

void _DrawPlanets(SDL_Renderer* renderer, GameState* game_state) {

    //load font for planet names
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        exit(1);
    }

    //planets will be drawn as just circles with their names on the top right
    for (int i = 0; i < game_state->n_planets; i++) {
        //draw planet i as a filled circle (blue)
        filledCircleRGBA(renderer, (int)game_state->planets[i].position.x, (int)game_state->planets[i].position.y, (int)game_state->planets[i].radius, 0, 0, 255, 255);

        //draw planet name at top-right of planet
        //name is a single char + the amount of trash inside the planet
        char name_text[10];
        snprintf(name_text, sizeof(name_text), "%c%d", game_state->planets[i].name, game_state->planets[i].trash_amount);
        SDL_Color textColor = {0, 0, 0, 255}; //black color

        SDL_Surface* textSurface = TTF_RenderText_Solid(font, name_text, textColor);
        if (textSurface == NULL) {
            printf("Failed to create text surface: %s\n", TTF_GetError());
            exit(1);
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == NULL) {
            printf("Failed to create text texture: %s\n", SDL_GetError());
            SDL_FreeSurface(textSurface);
            exit(1);
        }

        SDL_Rect textRect;
        textRect.x = (int)(game_state->planets[i].position.x + game_state->planets[i].radius);
        textRect.y = (int)(game_state->planets[i].position.y - game_state->planets[i].radius);
        textRect.w = textSurface->w * 2;
        textRect.h = textSurface->h * 2;
        SDL_FreeSurface(textSurface);
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
    }

    TTF_CloseFont(font);
}

void _DrawTrash(SDL_Renderer* renderer, GameState* game_state) {
    
    //trash will be drawn as small red circles
    for (int i = 0; i < game_state->n_trashes; i++) {
        filledCircleRGBA(renderer, (int)game_state->trashes[i].position.x, (int)game_state->trashes[i].position.y, (int)game_state->trashes[i].radius, 255, 0, 0, 255);
    }
}

void _DrawShips(SDL_Renderer* renderer, GameState* game_state) {

    //load font for planet names
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        exit(1);
    }

    for (int i = 0; i < game_state->max_ships; i++) {
        if (!game_state->ships[i].is_active) {
            return;
            //draw ship as a small green circle
        }
        

        SDL_Rect dst; SDL_Point point;
        dst.w = 100;
        dst.h = 50;
        dst.x = game_state->ships[i].Position.x - dst.w/2;
        dst.y = game_state->ships[i].Position.y - dst.h/2;
        
        point.x = dst.w/2;
        point.y = dst.h/2;

        Direction dir = game_state->ships[i].direction;
        
        double angle =  (dir == RIGHT)  ?   0   :
                        (dir == DOWN)   ?   90  :
                        (dir == LEFT)   ?   180 :
                                            270 ;

        //printf("%p\n", game_state->ships[i].imageTexture);

        int status = SDL_RenderCopyEx(renderer, game_state->ships[i].imageTexture, NULL, &dst, angle, &point, SDL_FLIP_NONE);

        //if(status == -1) printf("Status: %s\n", SDL_GetError());

        //draw ship name at top-right of ship
        //name is a single char + the amount of trash inside the ship
        //ships have the same name as their planet
        char name_text[10];
        snprintf(name_text, sizeof(name_text), "%c%d", game_state->ships[i].planet_id, game_state->ships[i].current_trash);
        SDL_Color textColor = {0, 0, 0, 255}; //black color
        
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, name_text, textColor);
        if (textSurface == NULL) {
            printf("Failed to create text surface: %s\n", TTF_GetError());
            exit(1);
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == NULL) {
            printf("Failed to create text texture: %s\n", SDL_GetError());
            SDL_FreeSurface(textSurface);
            exit(1);
        }

        SDL_Rect textRect;
        textRect.x = (int)(game_state->ships[i].Position.x + game_state->ships[i].radius);
        textRect.y = (int)(game_state->ships[i].Position.y - game_state->ships[i].radius);
        textRect.w = textSurface->w * 2;  
        textRect.h = textSurface->h * 2;
        SDL_FreeSurface(textSurface);
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
    }
    TTF_CloseFont(font);
}

void _DrawGameOver(SDL_Renderer* renderer, GameState* game_state) {

    if(game_state->is_game_over == 0) {
        return;
    }

    //load font for game over text
    TTF_Font* font = TTF_OpenFont("arial.ttf", 48);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        exit(1);
    }

    //create "Game Over" text surface
    SDL_Color textColor = {255, 0, 0, 255}; //red color
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, "GAME OVER", textColor);
    if (textSurface == NULL) {
        printf("Failed to create text surface: %s\n", TTF_GetError());
        exit(1);
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL) {
        printf("Failed to create text texture: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        exit(1);
    }

    SDL_Rect textRect;
    textRect.x = (game_state->universe_size - textSurface->w) / 2;
    textRect.y = (game_state->universe_size - textSurface->h) / 2;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);

    TTF_CloseFont(font);
}

void Draw(SDL_Renderer* renderer, GameState* game_state) {

    //set background color to whiteish gray
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderClear(renderer);

    //for each of the GameStates object vectors, we make a draw loop.
    _DrawPlanets(renderer, game_state);
    _DrawTrash(renderer, game_state);
    _DrawShips(renderer, game_state);
    _DrawGameOver(renderer, game_state);

    //present the rendered frame
    SDL_RenderPresent(renderer);
}

int checkQuit(){
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
        if(event.type == SDL_QUIT) return 1;

    return 0;
    
}

uint8_t checkKeyboard(int close){
    uint8_t msg;
    const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);
    if (close == 1){
        msg = ISDC;
    } else if(currentKeyStates[SDL_SCANCODE_W]||currentKeyStates[SDL_SCANCODE_UP]){
        msg = MYUP;
    } else if(currentKeyStates[SDL_SCANCODE_A]||currentKeyStates[SDL_SCANCODE_LEFT]){
        msg = MYLEFT;
    } else if(currentKeyStates[SDL_SCANCODE_S]||currentKeyStates[SDL_SCANCODE_DOWN]){
        msg = MYDOWN;
    } else if(currentKeyStates[SDL_SCANCODE_D]||currentKeyStates[SDL_SCANCODE_RIGHT]){
        msg = MYRIGHT;
    } else {
        msg = MYSTILL;
    }
    return msg;
}

SDL_Texture *getTexture(SDL_Renderer *renderer, const char *file,  gful_lifo **graceful_lifo){
    SDL_Surface *imageSurface= safe_IMG_Load(file, graceful_lifo);
    SDL_Texture *imageTexture = safe_SDL_CreateTextureFromSurface(renderer, imageSurface, graceful_lifo);
    closeSingleContext(graceful_lifo);
    return imageTexture;
}


    SDL_Window *safe_SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint8 flags, gful_lifo **graceful_lifo){
        SDL_Window *pWin = SDL_CreateWindow(title, x, y, w, h, flags);

        if(!pWin){
            printf("error initializing SDL: %s\n", SDL_GetError());
            closeContexts(*graceful_lifo);
            exit(1);
        }
        createContextDataforClosing(SDL_DestroyWindow, pWin, graceful_lifo);
        return pWin;
    }

    void safe_SDL_Init(Uint32 flags, gful_lifo **gracefull_lifo){
        if(SDL_Init(flags) != 0){
            printf("error initializing SDL: %s\n", SDL_GetError());
            closeContexts(*gracefull_lifo);
            exit(1);
        }
        createContextDataforClosing(SDL_Quit, NULL, gracefull_lifo);
    }

    void safe_TTF_Init(gful_lifo **graceful_lifo){
        if (TTF_Init() != 0) {
            printf("TTF_Init Error: %s\n", TTF_GetError());
            closeContexts(*graceful_lifo);
            exit(1);
        }
    }

    SDL_Renderer *safe_SDL_CreateRenderer(  SDL_Window * window,
                                            int index, Uint32 flags,
                                            gful_lifo **graceful_lifo){

        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
                                    
        if (renderer == NULL) {
            printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
            closeContexts(*graceful_lifo);
            exit(1);
        }
        createContextDataforClosing(SDL_DestroyRenderer, renderer, graceful_lifo);
        return renderer;
    }

    SDL_Surface *safe_IMG_Load(const char *file, gful_lifo **graceful_lifo){
        SDL_Surface *imageSurface= IMG_Load(file);
        if(imageSurface == NULL){
            printf("Failed to open image file.\n");
            closeContexts(*graceful_lifo);
            exit(1);
        }

        createContextDataforClosing(SDL_FreeSurface, imageSurface, graceful_lifo);
        return imageSurface;
    }

    SDL_Texture *safe_SDL_CreateTextureFromSurface(SDL_Renderer *renderer, SDL_Surface *SDLSurface, gful_lifo **graceful_lifo){
        SDL_Texture *imageTexture = SDL_CreateTextureFromSurface(renderer, SDLSurface);
        if(!imageTexture){
            printf("Failed to create texture from Surface: %s\n", SDL_GetError());
            closeContexts(*graceful_lifo);
            exit(1);
        }
        return imageTexture;
    }


SDL_Window *createClientWindow(gful_lifo **gracefull_lifo){
    return safe_SDL_CreateWindow("ShipClient",
                                        SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED,
                                        100, 100, 0,
                                        gracefull_lifo);
}

SDL_Window *createServerWindow(GameState *game_state, gful_lifo **gracefull_lifo){
    return safe_SDL_CreateWindow(   "Universe Simulator", 
                                    SDL_WINDOWPOS_CENTERED, 
                                    SDL_WINDOWPOS_CENTERED, 
                                    game_state->universe_size, 
                                    game_state->universe_size, 
                                    SDL_WINDOW_SHOWN,
                                    gracefull_lifo);
}

SDL_Renderer *createServerRenderer(SDL_Window *window, gful_lifo **gracefull_life){
    return safe_SDL_CreateRenderer( window, -1, 
                                    SDL_RENDERER_ACCELERATED, 
                                    gracefull_life);
}
