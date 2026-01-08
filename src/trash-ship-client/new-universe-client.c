#include <libconfig.h>
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_ttf.h>

#include <stdlib.h>
#include <stdio.h>
//#include <string.h>
//#include <pthread.h>


//#include "../Common/new-Communication.h"
#include "graceful-exit.h"


typedef struct config{
    int universeSize;
    int serverAddr;
    int serverBroadcast;
}config;

/*
Uint32 timer_callback(Uint32 interval, void *param){
    (void)param;
    SDL_Event timer_event;
    //printf("Timer callback function\n");

    SDL_zero(timer_event);
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 2;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}
*/

/*
pthread_mutex_t mutex_renderer = PTHREAD_MUTEX_INITIALIZER;
*/

int main(int argc, char *argv[]){
//Remove warning
    (void)argc;
    (void)argv;


//Program state
    gful_lifo *lastPosition = GFUL_INIT;
    //int end = 0;


//Read config file
    config_t cfg;
    config_init(&cfg);
    createContextDataforClosing((genericfunction *)config_destroy, &cfg, &lastPosition);
    if (!config_read_file(&cfg, "./trash-ship-client/client_config.conf"))
    {
        fprintf(stderr, "error\n");
        closeContexts(lastPosition);
        exit(1);
    }

    config myConfig;
    config_lookup_int(  &cfg, 
                        "universe.size", 
                        &myConfig.universeSize);
    
    config_lookup_int(   &cfg,
                        "comunication.rep_port",
                        &myConfig.serverAddr);

    config_lookup_int(   &cfg,
                        "comunication.pub_port",
                        &myConfig.serverBroadcast);

    printf("universe.size: %d\n", myConfig.universeSize);
    printf("comunication.rep_port: %d\n", myConfig.serverAddr);
    printf("comunication.pub_port: %d\n", myConfig.serverBroadcast);
    /*
    //create a string for the ports
    char rep_port_str[24];
    char pub_port_str[24];

    snprintf(rep_port_str, sizeof(rep_port_str), "tcp://localhost:%d", myConfig.serverAddr);
    snprintf(pub_port_str, sizeof(pub_port_str), "tcp://localhost:%d", myConfig.serverBroadcast);

    printf("comunication.serverAddress: %s\n", rep_port_str);
    printf("comunication.serverBroadcast: %s\n", pub_port_str);
    */
/*
//Initialize ZMQ
    void *zmqCtx = safe_zmq_ctx_new(&lastPosition);

//Public server address interaction
    void *zmqREQSocket = safe_zmq_socket(zmqCtx, ZMQ_REQ, &lastPosition);
    if (!zmqREQSocket)
    {
        closeContexts(lastPosition);
        return -1;
    }

    int connectStatus = zmq_connect(zmqREQSocket, (const char*)myConfig.serverAddr);
    if(connectStatus){
        printf("Error conecting to public server socket: %s\n", zmq_strerror(errno));
        closeContexts(lastPosition);
        return -1;
    }

    Server *serverRep = join_request(zmqREQSocket);
    if(!serverRep){
        closeContexts(lastPosition);
        return -1;
    }

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
//Create update window timer
    SDL_TimerID updateWindow = 0;
    updateWindow = SDL_AddTimer(30, (SDL_TimerCallback)timer_callback, NULL);

    SDL_Event SDL_tempEvent;

    while (!end)
    {
        SDL_WaitEvent(&SDL_tempEvent);
        switch (SDL_tempEvent.type)
        {
        case SDL_QUIT:
            end = 1;
            break;

        case SDL_USEREVENT:
            if(SDL_tempEvent.user.code == 2){
                pthread_mutex_lock(&mutex_renderer);
                SDL_RenderPresent(renderer);
                pthread_mutex_unlock(&mutex_renderer);

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
                com_ReqClient(zmqREQSocket, serverRep->key, SDL_tempEvent.type, SDL_tempEvent.key.keysym.scancode);
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
    }

    SDL_RemoveTimer(updateWindow);
    pthread_join(threadId, NULL); //For correct thread resource closing

    //Do Stuff
    server__free_unpacked(serverRep,NULL);
    */
    closeContexts(lastPosition);

    return 0;
}
