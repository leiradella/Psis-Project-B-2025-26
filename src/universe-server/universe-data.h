#ifndef UNIVERSE_DATA_H
#define UNIVERSE_DATA_H

#include <libconfig.h>

//default values for universe objects
#define PLANET_MASS 10
#define PLANET_RADIUS 20.0f
#define INITIAL_TRASH_AMOUNT 0
#define MIN_PLANET_DISTANCE 80.0f

#define TRASH_MASS 1
#define TRASH_RADIUS 4.0f

#define COLLISION_DISTANCE 1.0f

//math constants
#define PI 3.14159265f

//unvierse configuration structure
typedef struct UniverseConfig {
    int universe_size;
    int n_planets;
    int max_trash;
    int starting_trash;
    int trash_ship_capacity;
} UniverseConfig;

//universe objects and other structures
//position structure
typedef struct Position {
    float x;
    float y;
} Position;

//vector structure (used for velocities)
typedef struct Vector {
    float amplitude;
    float angle;
} Vector;

//planet structure
typedef struct Planet {
    char name;
    Position position;
    int mass;
    float radius;
    int trash_amount;
} Planet;

//trash structure
typedef struct Trash {
    Position position;
    Vector velocity;
    Vector acceleration;
    int mass;
    float radius;
} Trash;

//game state structure (so that we dont pass too many parameters on the main loop)
typedef struct GameState {
    int universe_size;
    Planet* planets;
    int n_planets;
    Trash *trashes;
    int n_trashes;
    int max_trash;

    int is_game_over;
} GameState;

//Vector creation from x and y components
Vector MakeVector(float x, float y);

//Vector addition
Vector AddVectors(Vector v1, Vector v2);

//this function reads universe parameters from the config file with name config_name
//and stores them in universe_config
int _GetUniverseParameters(const char* config_name, UniverseConfig* universe_config);

//this function looks up and integer from the config file cfg with name path and stores it in value
//returns 0 on success, 1 on failure printing an error message if any occurs
int _LookupUniverseInt(config_t *cfg, int *value, const char* path);

//this function reads all universe data from the config file cfg and stores it in universe_config
//returns the number of errors occurred during the process
int _ReadUniverseData(config_t *cfg, UniverseConfig *universe_config);

//helper to check if a point is valid (far enough from all existing points)
int _IsValidPosition(float x, float y, float* points_x, float* points_y, int num_points, float min_dist);

//position all planets randomly within the universe size using the seed value
//while making sure it does not overlap with other planets
//this function receives the vector with all planets to check for overlaps
//planets not placed yet have invalid positions (-100.0f, -100.0f)
void _PositionPlanets(Planet* planets, int n_planets, int universe_size, int seed);

//initialize all planets with default values (mass, radius, name, trash amount)
Planet* _InitializePlanets(int n_planets, int universe_size, int seed);

//position all trash randomly within the universe size using the seed value (like _PositionPlanets)
void _PositionTrash(Trash* trashes, int n_trashes, int universe_size, int seed);

//initialize all trash with default values (mass, radius, position, velocity)
Trash* _InitializeTrash(int n_trashes, int universe_size, int seed);

//create the universe initial state using the universe configuration
//this function is the only public function related to universe data reading
GameState* CreateInitialUniverseState(const char* config_name, int seed);

//DESTROY THE UNIVERSE (and free all allocated memory)
void DestroyUniverse(GameState** game_state);

#endif
