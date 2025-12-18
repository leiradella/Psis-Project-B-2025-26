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
        if (game_state->n_trashes >= game_state->max_trash) {
            continue; //skip collision if at max trash
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
    (void)game_state;
}

void _UpdateTrash(GameState* game_state) {

    _NewTrashAcceleration(game_state);
    _NewTrashVelocity(game_state);
    _NewTrashPosition(game_state);

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
    _UpdateTrash(game_state);

    _CheckGameOver(game_state);
}
