#include <libconfig.h>
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_ttf.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include <pthread.h>

#include "../universe-server/display.h"
#include "../universe-server/universe-data.h"
#include "Communication.h"
#include "graceful-exit.h"

typedef struct config
{
    int universeSize;
    int serverAddr;
    int serverBroadcast;
} config;

pthread_mutex_t mutex_renderer = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t fake_mutex = PTHREAD_MUTEX_INITIALIZER;

Uint32 timer_callback(Uint32 interval, void *param)
{
    (void)param;
    SDL_Event timer_event;
    // printf("Timer callback function\n");

    SDL_zero(timer_event);
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 2;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

typedef struct pthread_joinargs
{
    pthread_t id;
    void *output;
} pthread_joinArgs;

typedef struct client_thread_sub_arguements
{
    void *sub_port_str;
    void *zmq_ctx;
    GameState *sharedData;
    volatile int *end;
} client_thread_arguements;

typedef struct free_unpacked_args
{
    ServerMessage *message;
    ProtobufCAllocator *allocator;
} free_unpacked_args;

typedef struct SDL_RemoveTimer_args
{
    SDL_TimerID timer;
} SDL_RemoveTimer_args;

typedef struct TTF_CloseFont_args
{
    TTF_Font *font;
} TTF_CloseFont_args;
// Wrapper to make pthread_join compatible with your genericfunction signature -- gemini voodoo
// Nota para o professor como decidimos não retirar o pedantic e não queríamos uma nova linha
// no if do graceful esta função permite a correta execução do código.
void wrapper_pthread_join(void *arg)
{
    pthread_joinArgs *Data = (pthread_joinArgs *)arg;
    int result = pthread_join(Data->id, (void **)Data->output);

    if (result != 0)
    {
        printf("Error: pthread_join failed with code %d\n", result);
    }
    else
    {
        printf("Thread joined successfully.\n");
    }
}

void wrapper_server_message_free_unpacked(void *arg)
{
    free_unpacked_args *Data = (free_unpacked_args *)arg;
    server_message__free_unpacked(Data->message, Data->allocator);
}

void wrapper_SDL_remove_timer(void *arg)
{
    SDL_RemoveTimer_args *timer = (SDL_RemoveTimer_args *)arg;
    SDL_RemoveTimer(timer->timer);
}

void wrapper_TTF_CloseFont(void *arg)
{
    TTF_CloseFont_args *data = (TTF_CloseFont_args *)arg;
    TTF_CloseFont(data->font);
}

void *client_thread_sub(void *arg)
{
    client_thread_arguements *data = (client_thread_arguements *)arg;
    GameState *GameData = data->sharedData;
    gful_lifo *lastPosition2 = GFUL_INIT;

    void *zmqSubSocket = safe_zmq_socket(data->zmq_ctx, ZMQ_SUB, &lastPosition2);
    if (!zmqSubSocket)
    {
        printf("Critical error: Failed to create the Sub Socket.\n");
        *data->end = 1;
        return NULL;
    }
    zmq_msg_t zmq_sub;
    createContextDataforClosing((genericfunction *)zmq_msg_close, &zmq_sub, &lastPosition2);

    int timeout = 1500;
    int setSocket = zmq_setsockopt(zmqSubSocket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    if (setSocket)
    {
        printf("Critical Error: Failed to set timeout to Sub Socket.\n");
        *data->end = 1;
        closeContexts(lastPosition2);
        return NULL;
    }

    setSocket = zmq_setsockopt(zmqSubSocket, ZMQ_SUBSCRIBE, "", 0);
    if (setSocket)
    {
        printf("Critical Error: Failed to subscribe to server.\n");
        *data->end = 1;
        closeContexts(lastPosition2);
        return NULL;
    }

    int connectStatus = zmq_connect(zmqSubSocket, data->sub_port_str);
    if (connectStatus)
    {
        printf("Critical Error: Failed conecting to sub socket: %s\n", zmq_strerror(errno));
        *data->end = 1;
        closeContexts(lastPosition2);
        return NULL;
    }
    zmq_disconnectArgs args = {zmqSubSocket, data->sub_port_str};
    createContextDataforClosing((genericfunction *)zmq_disconnect, &args, &lastPosition2);

    while (!(*data->end))
    {
        // printf("%d\n", *data->end);
        int status = safe_zmq_msg_recv(zmqSubSocket, &zmq_sub, 0);

        if (status)
        {
            printf("Warning: Message timeout.\n");
        }
        else
        {
            UniverseStateMessage *serverPublish = zmq_msg_t_To_UniverseStateMessage(&zmq_sub);
            if (!serverPublish)
            {
                printf("Warning: Invalid pointer to zmq_msg_t.\n");
            }
            else
            {
                pthread_mutex_lock(&mutex_renderer);

                GameData->bg_a = serverPublish->bg_a;
                GameData->bg_b = serverPublish->bg_b;
                GameData->bg_g = serverPublish->bg_g;
                GameData->bg_r = serverPublish->bg_r;
                GameData->is_game_over = serverPublish->game_over;
                GameData->n_planets = (int)serverPublish->n_planets;
                GameData->n_ships = (int)serverPublish->n_ships;
                GameData->n_trashes = (int)serverPublish->n_trash_pieces;
                GameData->recycler_planet_index = serverPublish->planets[0]->recycler_index;

                if (GameData->planets)
                {
                    free(GameData->planets);
                }
                GameData->planets = (Planet *)malloc(sizeof(Planet) * serverPublish->n_planets);
                for (int i = 0; i < (int)serverPublish->n_planets; i++)
                {
                    int n = sscanf(serverPublish->planets[i]->name, "%c%d", &GameData->planets[i].name, &GameData->planets[i].trash_amount);

                    if (n != 2)
                    {
                        // Unsuccessfully extracted both values
                        printf("Warning: Failed to parse planet %d name.\n", i);
                        GameData->planets[i].name = 'A';
                        GameData->planets[i].trash_amount = 0;
                    }
                    GameData->planets[i].radius = PLANET_RADIUS;
                    GameData->planets[i].position.x = serverPublish->planets[i]->x;
                    GameData->planets[i].position.y = serverPublish->planets[i]->y;
                }

                if (GameData->ships)
                {
                    free(GameData->ships);
                }
                GameData->ships = (Ship *)malloc(sizeof(Ship) * serverPublish->n_ships);
                for (int i = 0; i < (int)serverPublish->n_ships; i++)
                {
                    int n = sscanf(serverPublish->ships[i]->name, "%c%d", &GameData->ships[i].name, &GameData->ships[i].trash_amount);

                    if (n != 2)
                    {
                        // Unsuccessfully extracted both values
                        printf("Warning: Failed to parse ship %d name.\n", i);
                        GameData->ships[i].name = 'A';
                        GameData->ships[i].trash_amount = 0;
                    }
                    GameData->ships[i].enabled = serverPublish->ships[i]->enable;
                    GameData->ships[i].position.x = serverPublish->ships[i]->x;
                    GameData->ships[i].position.y = serverPublish->ships[i]->y;
                    GameData->ships[i].radius = SHIP_RADIUS;
                }

                if (GameData->trashes)
                {
                    free(GameData->trashes);
                }
                GameData->trashes = (Trash *)malloc(sizeof(Trash) * serverPublish->n_trash_pieces);
                for (int i = 0; i < (int)serverPublish->n_trash_pieces; i++)
                {
                    GameData->trashes[i].position.x = serverPublish->trash_pieces[i]->x;
                    GameData->trashes[i].position.y = serverPublish->trash_pieces[i]->y;
                    GameData->trashes[i].radius = TRASH_RADIUS;
                }

                /*
            printf("Received the following data.\n");
            printf("GameOver: %d\n", serverPublish->game_over);
            printf("Number of planets: %zu\n", serverPublish->n_planets);
            printf("Planet name: %s\n", serverPublish->planets[0]->name);
            */

                pthread_mutex_unlock(&mutex_renderer);
            }
        }
    }

    closeContexts(lastPosition2);
    return NULL;
}

int main(int argc, char *argv[])
{
    // Remove warning
    (void)argc;
    (void)argv;

    // Program state
    gful_lifo *lastPosition = GFUL_INIT;
    // int end = 0;

    // Read config file
    config_t cfg;
    config_init(&cfg);
    createContextDataforClosing((genericfunction *)config_destroy, &cfg, &lastPosition);
    if (!config_read_file(&cfg, "./trash-ship-client/client_config.conf"))
    {
        fprintf(stderr, "error\n");
        closeContexts(lastPosition);
        return -1;
    }

    config myConfig;
    config_lookup_int(&cfg,
                      "universe.size",
                      &myConfig.universeSize);

    config_lookup_int(&cfg,
                      "comunication.rep_port",
                      &myConfig.serverAddr);

    config_lookup_int(&cfg,
                      "comunication.pub_port",
                      &myConfig.serverBroadcast);

    printf("universe.size: %d\n", myConfig.universeSize);

    // create a string for the ports
    char req_port_str[24];
    char sub_port_str[24];

    snprintf(req_port_str, sizeof(req_port_str), "tcp://localhost:%d", myConfig.serverAddr);
    snprintf(sub_port_str, sizeof(sub_port_str), "tcp://localhost:%d", myConfig.serverBroadcast);

    printf("comunication.serverAddress: %s\n", req_port_str);
    printf("comunication.serverBroadcast: %s\n", sub_port_str);

    // Initialize ZMQ
    void *zmqCtx = safe_zmq_ctx_new(&lastPosition);

    pthread_t thread_id;
    GameState *gameData = (GameState *)malloc(sizeof(GameState));
    gameData->mutex_enable = fake_mutex;
    volatile int end = 0;
    client_thread_arguements p_args = {sub_port_str, zmqCtx, gameData, &end};
    pthread_create(&thread_id, NULL, client_thread_sub, &p_args);

    pthread_joinArgs joinArgs = {thread_id, NULL};
    createContextDataforClosing((genericfunction *)wrapper_pthread_join, &joinArgs, &lastPosition);

    // Public server address interaction

    void *zmqREQSocket = safe_zmq_socket(zmqCtx, ZMQ_REQ, &lastPosition);
    if (!zmqREQSocket)
    {
        printf("Critical error: Failed to create the Req Socket.\n");
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }

    int timeout = 1500;
    int setSocket = zmq_setsockopt(zmqREQSocket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    if (setSocket)
    {
        printf("Critical Error: Failed to set timeout to Req Socket.\n");
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }

    int connectStatus = zmq_connect(zmqREQSocket, req_port_str);
    if (connectStatus)
    {
        printf("Error conecting to req socket: %s\n", zmq_strerror(errno));
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }
    zmq_disconnectArgs args = {zmqREQSocket, req_port_str};
    createContextDataforClosing((genericfunction *)zmq_disconnect, &args, &lastPosition);

    ClientMessage reqMessage = CLIENT_MESSAGE__INIT;
    int status = client_message_send(zmqREQSocket, reqMessage, &lastPosition);

    if (status == -1)
    {
        printf("Critical failure: Failed to send connection Message.\n");
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }

    zmq_msg_t zmq_rep;
    createContextDataforClosing((genericfunction *)zmq_msg_close, &zmq_rep, &lastPosition);
    status = safe_zmq_msg_recv(zmqREQSocket, &zmq_rep, 0);

    if (status)
    {
        printf("Critical failure: Failed to receive connection Message.\n");
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }

    ServerMessage *serverReply = zmq_msg_t_To_server_message(&zmq_rep);
    free_unpacked_args serverMessageArgs = {serverReply, NULL};
    createContextDataforClosing((genericfunction *)wrapper_server_message_free_unpacked, (void *)&serverMessageArgs, &lastPosition);

    printf("Received the following data.\n");
    printf("Status: %d\n", serverReply->msg_type);
    printf("id: %s\n", serverReply->id);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
        end = 1;
        closeContexts(lastPosition);
        return 0;
    }
    createContextDataforClosing((genericfunction *)SDL_Quit, NULL, &lastPosition);

    // Initialize SDL_ttf
    if (TTF_Init() != 0)
    {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }
    createContextDataforClosing((genericfunction *)TTF_Quit, NULL, &lastPosition);

    // load font
    TTF_Font *font = TTF_OpenFont("./universe-server/arial.ttf", 12);
    if (font == NULL)
    {
        printf("Failed to load font: %s\n", TTF_GetError());
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }
    TTF_CloseFont_args cfont_args = {font};
    createContextDataforClosing((genericfunction *)wrapper_TTF_CloseFont, &cfont_args, &lastPosition);

    gameData->font = font;

    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "Universe Client",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        myConfig.universeSize,
        myConfig.universeSize,
        SDL_WINDOW_SHOWN);

    // check if window was created successfully
    if (window == NULL)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }
    createContextDataforClosing((genericfunction *)SDL_DestroyWindow, window, &lastPosition);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        end = 1;
        closeContexts(lastPosition);
        return -1;
    }
    createContextDataforClosing((genericfunction *)SDL_DestroyRenderer, renderer, &lastPosition);

    /*
        int dcStatus = zmq_disconnect(zmqREQSocket, (const char*)myConfig.serverAddr);
        if(dcStatus){
            printf("Error discontecting from public server address: %s\n", strerror(errno));
        }

    //Private server address setup
        connectStatus = zmq_connect(zmqREQSocket, (const char*)serverRep->movementaddr);
        if(connectStatus){
            printf("Error connecting to privete server address: %s \n", strerror(errno));
            closeContexts(lastPosition);
            return -1;
        }


    //Brodcast server address setup
        void *zmqSocketSub = safe_zmq_socket(zmqCtx, ZMQ_SUB, &lastPosition);
        connectStatus = zmq_connect(zmqSocketSub, serverRep->broadcastaddr);
        if(connectStatus){
            printf("Error connecting to publisher: %s\n", strerror(errno));
            closeContexts(lastPosition);
            return -1;
        }

        int setSocket = zmq_setsockopt(zmqSocketSub, ZMQ_SUBSCRIBE, "Data", 4);
        if(setSocket){
            printf("Error subscribing to data: %s\n", strerror(errno));
            closeContexts(lastPosition);
            return -1;
        }
        int timeout = 1500;
        setSocket = zmq_setsockopt(zmqSocketSub, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    //Initialize
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            printf("error initializing SDL: %s\n", SDL_GetError());
        }
        createContextDataforClosing((genericfunction *)SDL_Quit, NULL, &lastPosition);

    //Initialize SDL_ttf
        if (TTF_Init() != 0) {
            printf("TTF_Init Error: %s\n", TTF_GetError());
            closeContexts(lastPosition);
            return 1;
        }

    //Create window
        SDL_Window *window = SDL_CreateWindow(
            "Universe Simulator",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            myConfig.universeSize,
            myConfig.universeSize,
            SDL_WINDOW_SHOWN
        );

        //check if window was created successfully
        if (window == NULL) {
            printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
            closeContexts(lastPosition);
            return 1;
        }

    //create SDL_renderer
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        if (renderer == NULL) {
            SDL_DestroyWindow(window);
            printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
            closeContexts(lastPosition);
            return 1;
        }

    //Create thread to handle private connection
         pthread_t threadId;
         clientThreadArgs threadArgs = {&end, renderer, zmqSocketSub};
         int threadStatus = pthread_create(&threadId, NULL, com_threadClient, &threadArgs);
        if(threadStatus){
            printf("Error initializing the thread: %s\n", strerror(errno));
            closeContexts(lastPosition);
            return -1;
        }
            */
    // Create update window timer
    SDL_TimerID updateWindow = 0;
    updateWindow = SDL_AddTimer(30, (SDL_TimerCallback)timer_callback, NULL);
    SDL_RemoveTimer_args timerArgs = {updateWindow};
    createContextDataforClosing((genericfunction *)wrapper_SDL_remove_timer, &timerArgs, &lastPosition);
    SDL_Event SDL_tempEvent;

    while (!end)
    {
        SDL_WaitEvent(&SDL_tempEvent);
        switch (SDL_tempEvent.type)
        {
        case SDL_QUIT:
            printf("Quit Event\n");
            reqMessage.id = strdup(serverReply->id);
            reqMessage.msg_type = CLIENT_MESSAGE_TYPE__DISCONNECT;
            int status = client_message_send(zmqREQSocket, reqMessage, &lastPosition);

            if (status == -1)
            {
                printf("Warning: Failed to send disconnect Message.\n");
            }

            status = safe_zmq_msg_recv(zmqREQSocket, &zmq_rep, 0);

            if (status)
            {
                printf("Warning: Failed to receive disconnect Rep.\n");
            }

            ServerMessage *serverReplyDc = zmq_msg_t_To_server_message(&zmq_rep);
            free_unpacked_args serverMessageArgs = {serverReplyDc, NULL};
            createContextDataforClosing((genericfunction *)wrapper_server_message_free_unpacked, (void *)&serverMessageArgs, &lastPosition);

            printf("Received the following data.\n");
            printf("Status: %d\n", serverReplyDc->msg_type);
            printf("id: %s\n", serverReplyDc->id);

            closeSingleContext(&lastPosition);
            end = 1;
            break;

        case SDL_USEREVENT:
            if (SDL_tempEvent.user.code == 2)
            {
                if (gameData->ships)
                {
                    pthread_mutex_lock(&mutex_renderer);
                    Draw(renderer, gameData);
                    //printf("Ship name:%c\n", gameData->ships[0].name);
                    pthread_mutex_unlock(&mutex_renderer);
                    // end = 1;
                }
            }
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            switch (SDL_tempEvent.key.keysym.scancode)
            {
            case SDL_SCANCODE_W:
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
            case SDL_SCANCODE_S:
            case SDL_SCANCODE_DOWN:
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                // com_ReqClient(zmqREQSocket, serverRep->key, SDL_tempEvent.type, SDL_tempEvent.key.keysym.scancode);
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
        client_message__init(&reqMessage);
    }

    // SDL_Delay(20000);
    //  Do Stuff
    closeContexts(lastPosition);
    free(gameData->planets);
    free(gameData->ships);
    free(gameData->trashes);
    free(gameData);
    return 0;
}
