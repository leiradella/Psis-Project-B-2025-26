#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libconfig.h>

#include "universe-data.h"

Vector MakeVector(float x, float y) {
    Vector result;
    result.amplitude = sqrt(x * x + y * y);
    result.angle = atan2(y, x) * (180.0f / PI); //convert to degrees
    return result;
}

Vector AddVectors(Vector v1, Vector v2) {
    //convert to coordinates
    float x1 = v1.amplitude * cos(v1.angle * (PI / 180.0f));
    float y1 = v1.amplitude * sin(v1.angle * (PI / 180.0f));
    float x2 = v2.amplitude * cos(v2.angle * (PI / 180.0f));
    float y2 = v2.amplitude * sin(v2.angle * (PI / 180.0f));

    //add components
    float x_total = x1 + x2;
    float y_total = y1 + y2;

    //convert back to polar coordinates
    return MakeVector(x_total, y_total);
}

int _GetUniverseParameters(const char* config_name, UniverseConfig* universe_config) {
    //init and read config file
    config_t cfg;
    config_init(&cfg);

    if (config_read_file(&cfg, config_name) == CONFIG_FALSE) {
        printf("Error in %s at line %d: %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return 1;
    }

    //read universe data from config file
    if (_ReadUniverseData(&cfg, universe_config) != 0) {
        config_destroy(&cfg);
        return 1;
    }

    config_destroy(&cfg);
    return 0;
}

int _LookupUniverseInt(config_t *cfg, int *value, const char* path) {
    if (config_lookup_int(cfg, path, value) == CONFIG_FALSE) {
        printf("Error in %s at line %d: %s\n", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
        config_destroy(cfg);
        return 1;
    }
    return 0;
}

int _ReadUniverseData(config_t *cfg, UniverseConfig *universe_config) {

    //how many errors occurred
    int status = 0;

    status += _LookupUniverseInt(cfg, &universe_config->universe_size, "universe.size");
    status += _LookupUniverseInt(cfg, &universe_config->n_planets, "universe.n_planets");
    status += _LookupUniverseInt(cfg, &universe_config->max_trash, "universe.max_trash");
    status += _LookupUniverseInt(cfg, &universe_config->starting_trash, "universe.starting_trash");
    status += _LookupUniverseInt(cfg, &universe_config->trash_ship_capacity, "universe.trash_ship_capacity");
    return status;
}

int _IsValidPosition(float x, float y, float* points_x, float* points_y, int num_points, float min_dist) {
    for (int i = 0; i < num_points; i++) {
        float dx = points_x[i] - x;
        float dy = points_y[i] - y;
        float dist_sq = dx * dx + dy * dy;
        if (dist_sq < min_dist * min_dist) {
            return 0;
        }
    }
    return 1;
}

void _PositionPlanets(Planet* planets, int n_planets, int universe_size, int seed) {

    //coordinate variables
    float x, y;

    //loop flags
    int positioned = 0;
    int attempts = 0;
    const int max_attempts = 1000 * n_planets; //max attempts to position all planets

    //current planet index
    int current_planet = 0;

    while (!positioned && attempts < max_attempts) {
        attempts++;
        
        //get random coordinates within bounds (PLANET_RADIUS from edges)
        float range = universe_size - 2 * PLANET_RADIUS;
        x = PLANET_RADIUS + ((float)(rand_r((unsigned int*)&seed) % (int)range));
        y = PLANET_RADIUS + ((float)(rand_r((unsigned int*)&seed) % (int)range));

        //valid flag
        int valid = 1;

        //check if the position is valid (only check already-placed planets)
        for (int i = 0; i < current_planet; i++) {
            float dx = planets[i].position.x - x;
            float dy = planets[i].position.y - y;
            float dist_sq = dx * dx + dy * dy;

            if (dist_sq < MIN_PLANET_DISTANCE * MIN_PLANET_DISTANCE) {
                //position is invalid, break and try again
                valid = 0;
                break;
            }
        }

        if (valid) {
            //position is valid, assign it to the planet
            planets[current_planet].position.x = x;
            planets[current_planet].position.y = y;
            current_planet++;
        }

        if (current_planet >= n_planets) {
            //all planets positioned
            positioned = 1;
        }
    }
}

Planet *_InitializePlanets(int n_planets, int universe_size, int seed) {
    //create a vector with all planets
    Planet* planets = (Planet*)malloc(n_planets * sizeof(Planet));

    //initialize radius and mass constants, names, positions, and trash amounts
    for (int i = 0; i < n_planets; i++) {
        planets[i].mass = PLANET_MASS;
        planets[i].radius = PLANET_RADIUS;
        planets[i].trash_amount = INITIAL_TRASH_AMOUNT;
        planets[i].name = 'A' + i; //assign names like A B C...

        //initialize positions with invalid values
        planets[i].position.x = -100.0f;
        planets[i].position.y = -100.0f;
    }

    //position all planets
    _PositionPlanets(planets, n_planets, universe_size, seed);

    return planets;
}

void _PositionTrash(Trash* trashes, int n_trashes, int universe_size, int seed) {

    //coordinate variables
    float x, y;

    //loop flags
    int positioned = 0;
    int attempts = 0;
    const int max_attempts = 1000 * n_trashes; //max attempts to position all trash

    //current trash index
    int current_trash = 0;

    while (!positioned && attempts < max_attempts) {
        attempts++;
        
        //get random coordinates within bounds (TRASH_RADIUS from edges)
        float range = universe_size - 2 * TRASH_RADIUS;
        x = TRASH_RADIUS + ((float)(rand_r((unsigned int*)&seed) % (int)range));
        y = TRASH_RADIUS + ((float)(rand_r((unsigned int*)&seed) % (int)range));

        //assign position to trash
        trashes[current_trash].position.x = x;
        trashes[current_trash].position.y = y;
        current_trash++;

        if (current_trash >= n_trashes) {
            //all trash positioned
            positioned = 1;
        }
    }
}

Trash *_InitializeTrash(int n_trashes, int universe_size, int seed) {
    
    //create trash vector
    Trash *trashes = (Trash*)malloc(n_trashes * sizeof(Trash));

    //initialize trash properties
    for (int i = 0; i < n_trashes; i++) {
        trashes[i].mass = TRASH_MASS; 
        trashes[i].radius = TRASH_RADIUS;
        trashes[i].position.x = -100.0f;
        trashes[i].position.y = -100.0f;

        //initialize with random velocity
        trashes[i].velocity.amplitude = ((float)(rand_r((unsigned int*)&seed) % 100)) / 100.0f; //0.0 to 1.0
        trashes[i].velocity.angle = ((float)(rand_r((unsigned int*)&seed) % 360)); //0 to 359 degrees

        //no initial acceleration
        trashes[i].acceleration.amplitude = 0.0f;
        trashes[i].acceleration.angle = 0.0f;
    }

    //position all trash
    _PositionTrash(trashes, n_trashes, universe_size, seed);

    return trashes;
}

GameState *CreateInitialUniverseState(const char* config_name, int seed) {

    //create a universe config struct with the parameters
    UniverseConfig universe_config;

    //get the params with the config file
    _GetUniverseParameters(config_name, &universe_config);

    if (universe_config.n_planets <= 0 || universe_config.max_trash <= 0 || 
        universe_config.starting_trash < 0 || universe_config.starting_trash > universe_config.max_trash) {
        printf("Invalid universe configuration parameters.\n");
        return NULL;
    }

    //now we can use universe_config to create the initial state of the universe
    //the results are semi deterministic based on the seed value


    //initialize the planets (mass, radius, name, trash amount, position)
    Planet* planets = _InitializePlanets(universe_config.n_planets, universe_config.universe_size, seed);

    //initialize the trash
    Trash *trashes = _InitializeTrash(universe_config.max_trash, universe_config.universe_size, seed);

    //gamestate struct to return
    GameState* game_state = malloc(sizeof(GameState));
    game_state->universe_size = universe_config.universe_size;
    game_state->planets = planets;
    game_state->n_planets = universe_config.n_planets;
    game_state->trashes = trashes;
    game_state->n_trashes = universe_config.starting_trash;
    game_state->max_trash = universe_config.max_trash;
    game_state->is_game_over = 0;

    return game_state;
}

void DestroyUniverse(GameState** game_state) {
    free((*game_state)->planets);
    free((*game_state)->trashes);
    free(*game_state);
    *game_state = NULL;
}
