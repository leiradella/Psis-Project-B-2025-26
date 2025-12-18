#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "universe-data.h"

//internal draw functions for each object type in the game state
void _DrawPlanets(SDL_Renderer* renderer, GameState* game_state);
void _DrawTrash(SDL_Renderer* renderer, GameState* game_state);

//draw function
void Draw(SDL_Renderer* renderer, GameState* game_state);

#endif
