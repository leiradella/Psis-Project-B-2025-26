#include "physics-rules.h"
#include <SDL2/SDL.h>

void CheckEvents(int* running, GameState* game_state) {

    (void)game_state; //currently unused

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            printf("Quit event received. Exiting...\n");
            *running = 0;
        }
    }
}

int _IsAnyShipActive(GameState* game_state) {
    //only generate new trash when at least 1 ship is active
    int active_ships = 0;
    for (int i=0; i<game_state->n_ships;i++) {
        active_ships += game_state->ships[i].enabled;
    }
    if (active_ships <= 0) {
        return 0;
    }
    return 1;
}

void _NewTrashAcceleration(GameState* game_state) {
    Vector total_vector_force;
    Vector local_vector_force;

    for (int i = 0; i < game_state->n_trashes; i ++) {
        total_vector_force.amplitude = 0;
        total_vector_force.angle = 0;
        for (int n_planet = 0; n_planet < game_state->n_planets; n_planet ++){
            float force_vector_x = game_state->planets[n_planet].position.x - game_state->trashes[i].position.x;
            float force_vector_y = game_state->planets[n_planet].position.y - game_state->trashes[i].position.y;
            local_vector_force = MakeVector(force_vector_x, force_vector_y);
            local_vector_force.amplitude = game_state->planets[n_planet].mass / pow(local_vector_force.amplitude, 2);
            total_vector_force = AddVectors(local_vector_force, total_vector_force);
            game_state->trashes[i].acceleration = total_vector_force;
        }
    }
}

void _NewTrashVelocity(GameState* game_state) {
    for (int i = 0; i < game_state->n_trashes; i ++) {
        game_state->trashes[i].velocity.amplitude *= 0.99;
        game_state->trashes[i].velocity= AddVectors(game_state->trashes[i].velocity,
        game_state->trashes[i].acceleration);
    }
}

void _NewTrashPosition(GameState* game_state) {
    for (int i = 0; i < game_state->n_trashes; i ++) {
        
        float delta_x = game_state->trashes[i].velocity.amplitude * 
            cos(game_state->trashes[i].velocity.angle * (3.14159265f / 180.0f));

        float delta_y = game_state->trashes[i].velocity.amplitude * 
            sin(game_state->trashes[i].velocity.angle * (3.14159265f / 180.0f));

        game_state->trashes[i].position.x += delta_x;
        game_state->trashes[i].position.y += delta_y;

        //loop around the universe edges
        if (game_state->trashes[i].position.x < 0) {
            game_state->trashes[i].position.x += game_state->universe_size;
        }
        else if (game_state->trashes[i].position.x > game_state->universe_size) {
            game_state->trashes[i].position.x -= game_state->universe_size;
        }

        if (game_state->trashes[i].position.y < 0) {
            game_state->trashes[i].position.y += game_state->universe_size;
        }
        else if (game_state->trashes[i].position.y > game_state->universe_size) {
            game_state->trashes[i].position.y -= game_state->universe_size;
        }

        //planet collision
        if (game_state->n_trashes >= game_state->max_trash || !_IsAnyShipActive(game_state)) {
            continue; //skip collision if at max trash or if there arent any ships active
        }
        for (int j = 0; j < game_state->n_planets; j++) {
            float dx = game_state->trashes[i].position.x - game_state->planets[j].position.x;
            float dy = game_state->trashes[i].position.y - game_state->planets[j].position.y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < COLLISION_DISTANCE) {
                //create 2 trashes from the collision (reuse the current trash and create a new one)
                
                //add a new trash child into this dog gone universe
                game_state->trashes[game_state->n_trashes] = game_state->trashes[i];
                game_state->n_trashes++;

                //slightly change both their angles based on the collision
                game_state->trashes[i].velocity.angle += 15.0f;
                game_state->trashes[game_state->n_trashes - 1].velocity.angle -= 15.0f;
            }
        }
    }
}

void _UpdatePlanets(GameState* game_state) {
    
    //change recycle planets periodically
    Uint32 current_time = SDL_GetTicks();
    Uint32 interval_ms = game_state->planet_change_rate;

    if (current_time - game_state->last_planet_change_time >= interval_ms) {

        if (!_IsAnyShipActive(game_state) || game_state->n_planets < 2) {
            return; //dont change if no active ships or less than 2 planets
        }

        //change recycle planet to a new random one
        int new_index = game_state->recycler_planet_index;
        while(new_index == game_state->recycler_planet_index) {
            new_index = rand() % game_state->n_planets;
        }
        game_state->recycler_planet_index = new_index;
        game_state->last_planet_change_time = current_time;
    }
}

void _GenerateTrash(GameState* game_state) {

    //generate trash periodically 
    Uint32 current_time = SDL_GetTicks();
    Uint32 interval_ms = game_state->trash_gen_rate;
    
    //check if enough time has passed since last generation
    if (current_time - game_state->last_trash_gen_time >= interval_ms) {

        if (!_IsAnyShipActive(game_state)) {
            return;
        }

        //generate new trash if we haven't reached max
        if (game_state->n_trashes < game_state->max_trash) {
            //get random x and y inside universe bounds
            float x = ((float)(rand() % game_state->universe_size));
            float y = ((float)(rand() % game_state->universe_size));

            //place new trash at that position
            game_state->trashes[game_state->n_trashes].position.x = x;
            game_state->trashes[game_state->n_trashes].position.y = y;
            //get random velocity
            game_state->trashes[game_state->n_trashes].velocity.amplitude = ((float)(rand() % 100)) / 100.0f;
            game_state->trashes[game_state->n_trashes].velocity.angle = ((float)(rand() % 360));

            game_state->n_trashes++;
        }
        
        //update last generation time
        game_state->last_trash_gen_time = current_time;
    }
}

void _UpdateTrash(GameState* game_state) {

    _NewTrashAcceleration(game_state);
    _NewTrashVelocity(game_state);
    _NewTrashPosition(game_state);

}

void _NewShipAcceleration(GameState* game_state) {
    Vector total_vector_force;
    Vector local_vector_force;

    for (int i = 0; i < game_state->n_ships; i ++) {
        if (!game_state->ships[i].enabled) continue;
        total_vector_force.amplitude = 0;
        total_vector_force.angle = 0;
        for (int n_planet = 0; n_planet < game_state->n_planets; n_planet ++){
            float force_vector_x = game_state->planets[n_planet].position.x - game_state->ships[i].position.x;
            float force_vector_y = game_state->planets[n_planet].position.y - game_state->ships[i].position.y;
            local_vector_force = MakeVector(force_vector_x, force_vector_y);
            local_vector_force.amplitude = game_state->planets[n_planet].mass / pow(local_vector_force.amplitude, 2);
            total_vector_force = AddVectors(local_vector_force, total_vector_force);
            game_state->ships[i].acceleration = total_vector_force;
        }
    }
}

void _NewShipVelocity(GameState* game_state) {
    for (int i = 0; i < game_state->n_ships; i ++) {
        if (!game_state->ships[i].enabled) continue;
        game_state->ships[i].velocity.amplitude *= 0.99;
        game_state->ships[i].velocity= AddVectors(game_state->ships[i].velocity,
        game_state->ships[i].acceleration);
    }
}

void _NewShipPosition(GameState *game_state) {
    for (int i = 0; i < game_state->n_ships; i ++) {
        if (!game_state->ships[i].enabled) continue;
        
        float delta_x = game_state->ships[i].velocity.amplitude * 
            cos(game_state->ships[i].velocity.angle * (3.14159265f / 180.0f));

        float delta_y = game_state->ships[i].velocity.amplitude * 
            sin(game_state->ships[i].velocity.angle * (3.14159265f / 180.0f));
        game_state->ships[i].position.x += delta_x;
        game_state->ships[i].position.y += delta_y;

        //loop around the universe edges
        if (game_state->ships[i].position.x < 0) {
            game_state->ships[i].position.x = game_state->universe_size;
        }
        else if (game_state->ships[i].position.x > game_state->universe_size) {
            game_state->ships[i].position.x = game_state->universe_size;
        }

        if (game_state->ships[i].position.y < 0) {
            game_state->ships[i].position.y = game_state->universe_size;
        }
        else if (game_state->ships[i].position.y > game_state->universe_size) {
            game_state->ships[i].position.y = game_state->universe_size;
        }
    }    
}

void _UpdateShips(GameState* game_state) {

    _NewShipAcceleration(game_state);
    _NewShipVelocity(game_state);
    _NewShipPosition(game_state);
}

void _CheckGameOver(GameState* game_state) {

    //once the trash hits the max, its game over
    //were gonna make a text appear on screen
    if (game_state->n_trashes >= game_state->max_trash) {
        game_state->is_game_over = 1;
    }
}

void UpdateUniverse(GameState* game_state) {
    //update each object vector in the game state
    _UpdatePlanets(game_state);

    //trash stuff
    _GenerateTrash(game_state);
    _UpdateTrash(game_state);

    //ship stuff
    _UpdateShips(game_state);

    _CheckGameOver(game_state);
}
