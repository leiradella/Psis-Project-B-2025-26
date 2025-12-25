#include <stdlib.h>
#include <time.h>

#include <libconfig.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "universe-data.h"
#include "physics-rules.h"
#include "display.h"
#include "communication.h"

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

    //initialize communications
    CommunicationManager* comm = CommunicationInit(game_state); //cant have more clients than ships
    if (comm == NULL) {
        printf("Failed to initialize communication manager. Exiting...\n");
        DestroyUniverse(&game_state);
        return 1;
    }

    ReceiveClientConnection(comm); //test receiving client connections

    //Initalize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        DestroyUniverse(&game_state);
        CommunicationQuit(&comm);
        return 1;
    }

    //Initialize SDL_ttf
    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        DestroyUniverse(&game_state);
        CommunicationQuit(&comm);
        return 1;
    }

    //load font
    TTF_Font* font = TTF_OpenFont("arial.ttf", 12);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        SDL_Quit();
        DestroyUniverse(&game_state);
        CommunicationQuit(&comm);
        exit(1);
    }
    game_state->font = font;

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
        DestroyUniverse(&game_state);
        CommunicationQuit(&comm);
        return 1;
    }

    //create SDL_renderer variable
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL) {
        SDL_DestroyWindow(window);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        DestroyUniverse(&game_state);
        CommunicationQuit(&comm);
        return 1;
    }

    //main loop flag
    int running = 1;

    //timing variables for independent update and draw frequencies
    const int UPDATE_FPS = 100;           //updates per second (physics/logic)
    const int DRAW_FPS = 30;              //frames per second (rendering)
    const float UPDATE_INTERVAL = 1000.0f / (float)UPDATE_FPS;  //milliseconds per update
    const float DRAW_INTERVAL = 1000.0f / (float)DRAW_FPS;      //milliseconds per frame

    Uint32 last_update_time = SDL_GetTicks();
    Uint32 last_draw_time = SDL_GetTicks();

    //main loop: check events, update universe state, draw universe at independent rates
    while (running) {
        Uint32 current_time = SDL_GetTicks();

        //always check for events
        CheckEvents(&running, game_state);

        //update universe at UPDATE_FPS rate
        if (current_time - last_update_time >= UPDATE_INTERVAL) {
            UpdateUniverse(game_state);
            last_update_time = current_time;
        }

        //draw at DRAW_FPS rate
        if (current_time - last_draw_time >= DRAW_INTERVAL) {
            Draw(renderer, game_state);
            last_draw_time = current_time;
        }

        //calculate time until next event and sleep efficiently
        //update the update and draw times to reflect this sleep
        current_time = SDL_GetTicks();
        float time_to_next_update = UPDATE_INTERVAL - (current_time - last_update_time);
        float time_to_next_draw = DRAW_INTERVAL - (current_time - last_draw_time);

        float time_to_next_event;
        if (time_to_next_update < time_to_next_draw) {
            time_to_next_event = time_to_next_update;
        } else {
            time_to_next_event = time_to_next_draw;
        }
        
        if (time_to_next_event > 1) {
            SDL_Delay((Uint32)(time_to_next_event - 1));
        }
    }

    //free allocated universe state memory
    DestroyUniverse(&game_state);

    //cleanup comms
    CommunicationQuit(&comm);

    //destroy SDL variables and quit SDL
    TTF_CloseFont(game_state->font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    printf("Universe Simulator exited successfully.\n");
    return 0;
}
