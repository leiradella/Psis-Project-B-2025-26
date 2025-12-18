#include <stdlib.h>
#include <time.h>

#include <libconfig.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "universe-data.h"
#include "physics-rules.h"
#include "display.h"

int main(int argc, char* argv[]) {

    (void)argc; (void)argv; //unused

    //seed random number generator with current time
    srand(time(NULL));
    int seed = rand(); //initialize seed with a random value

    //create the universal initial state here using universe_config parameters
    GameState* game_state = CreateInitialUniverseState("universe_config.conf", seed);

    if (game_state == NULL) {
        printf("Failed to create initial universe state. Exiting...\n");
        return 1;
    }

    //Initalize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    //Initialize SDL_ttf
    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    

    //create SDL_window variable
    SDL_Window *window = SDL_CreateWindow(
        "Universe Simulator", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        game_state->universe_size, 
        game_state->universe_size, 
        SDL_WINDOW_SHOWN
    );

    //check if window was created successfully
    if (window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    //create SDL_renderer variable
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL) {
        SDL_DestroyWindow(window);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    //main loop flag
    int running = 1;

    //main loop: check events, update universe state, draw universe
    while (running) {
        CheckEvents(&running, game_state);
        UpdateUniverse(game_state);
        Draw(renderer, game_state);

        SDL_Delay(16); 
    }

    //free allocated universe state memory
    DestroyUniverse(&game_state);


    //destroy SDL variables and quit SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    printf("Universe Simulator exited successfully.\n");
    return 0;
}
