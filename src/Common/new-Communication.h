#ifndef NEW_COMMUNICATIONS_H
#define NEW_COMMUNICATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <zmq.h>
#include <SDL2/SDL.h>
#include "msgB.pb-c.h"

#include "universe-data.h"
#include "display.h"
//Descriptors

#define GETID UINT64_MAX

typedef struct clientData{
    uint64_t key;
    void *movAdd;
    void *broadcastAdd;
}clientData;

typedef struct clientThreadArgs{
    int *end;
    SDL_Renderer *renderer;
    void *socket;
} clientThreadArgs;

extern pthread_mutex_t mutex_renderer;

void *safe_zmq_ctx_new(gful_lifo **lastPosition);

void *safe_zmq_socket(void *zmq_ctx, int type_, gful_lifo **lastPosition);


//Function requests data needed for new client
//from the argument socket, and receives said data.
Server *join_request(void *s_);

//Function receives data from the server broadcast
//and presents it to the address described in the arguments
void *com_threadClient(void *arguments);

//Function that receives the event and socket data
//to send to the server the key press informataion.
int com_ReqClient(void *s_, uint64_t key, enum SDL_EventType tipo,  enum SDL_Scancode input);

//Function that receives the socket and identification data.
//to send to the server with the quit request
int com_QuitRecClient(void *s_, uint64_t key);

#endif
