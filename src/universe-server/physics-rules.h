#ifndef PHYSICS_RULES_H
#define PHYSICS_RULES_H

#include <SDL2/SDL.h>

#include "universe-data.h"

//check events function
void CheckEvents(int* running, GameState* game_state);

/*---------TRASH STUFF--------------*/
//Calculate new acceleration for each trash based on planet positions
void _NewTrashAcceleration(GameState* game_state);
//Calculate new velocity for each trash based on its acceleration
void _NewTrashVelocity(GameState* game_state);
//Calculate new position for each trash based on its velocity
void _NewTrashPosition(GameState* game_state);

//universe update function
void UpdateUniverse(GameState* game_state);

#endif
