#include "new-Communication.h"

#include "graceful-exit.h"

void *safe_zmq_ctx_new(gful_lifo **lastPosition){
    void *zmq_ctx = zmq_ctx_new();
    if(!zmq_ctx){
        printf("Error initializing ZMQ_ctx: %s\n", zmq_strerror(errno));
        closeContexts(*lastPosition);
        exit(1);
    }
    createContextDataforClosing((genericfunction *)zmq_close, zmq_ctx, lastPosition);
    return zmq_ctx;
}

void *safe_zmq_socket(void *zmq_ctx, int type_, gful_lifo **lastPosition){
    void *zmqSocket = zmq_socket(zmq_ctx, type_);
    if(!zmqSocket){
        printf("Error initializing ZMQ socket: %s\n", zmq_strerror(errno));
        return NULL;
    }
    createContextDataforClosing((genericfunction *)zmq_close, zmqSocket, lastPosition);
    return zmqSocket;
}

Server *join_request(void *s_){

//Prepare message
    Client joinReq = CLIENT__INIT;
    joinReq.type = ENUM_TYPE__joinRequest;

    const int ReqLen = client__get_packed_size(&joinReq);
    uint8_t *bufReq = (uint8_t *)malloc(ReqLen);

    client__pack(&joinReq, bufReq);

    if(zmq_send(s_, bufReq, ReqLen, 0) == -1)
    {
        fprintf(errno, "Failed to send join request.\n");
        return NULL;
    }

    zmq_msg_t joinRep;
    zmq_msg_init(&joinRep);

    int const RepLen = zmq_msg_recv(s_, &joinRep, 0);
    void *bufRep = zmq_msg_data(&joinRep);

    const Server * msgRep = client__unpack(NULL, RepLen, bufRep); //
    zmq_msg_close(&joinRep);

    return msgRep;
}

void *com_threadClient(void *argument){
    zmq_msg_t zmq_broadcastData;
    zmq_msg_init(&zmq_broadcastData);
    clientThreadArgs arguments = *(clientThreadArgs *)argument;
    int SubLen;
    UniverseData *proto_broadcastData;

    //Lopp
    while (!(*arguments.end))
    {
        SubLen = zmq_msg_recv(&zmq_broadcastData, arguments.socket, 0);
        if (SubLen == -1)
        {
            if(errno == EAGAIN){
                printf("Server timed out: %s\n", strerror(errno));
                *arguments.end = 1;
            }

        } else {
        
            void *bufRep = zmq_msg_data(&zmq_broadcastData);

            proto_broadcastData = universe_data__unpack(NULL,SubLen,bufRep);

            GameState *Game_state;
            Game_state->planets = proto_broadcastData->planets.data;
            Game_state->n_planets = proto_broadcastData->planets.len / sizeof(Planet);
            Game_state->ships = proto_broadcastData->ships.data;
            Game_state->n_ships = proto_broadcastData->ships.len / sizeof(Ship);
            Game_state->trashes = proto_broadcastData->trash.data;
            Game_state->n_trashes = proto_broadcastData->trash.len / sizeof(Trash);
            Game_state->is_game_over = proto_broadcastData->isover;

            pthread_mutex_lock(&mutex_renderer);
            SDL_Renderer *renderer = arguments.renderer;
            //set background color to whiteish gray
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderClear(renderer);

            //for each of the GameStates object vectors, we make a draw loop.
            _DrawPlanets(renderer, Game_state);
            _DrawTrash(renderer, Game_state);
            _DrawShips(renderer, Game_state);
            _DrawGameOver(renderer, Game_state);
            pthread_mutex_unlock(&mutex_renderer);

            universe_data__free_unpacked(proto_broadcastData, NULL);
        }
    }
    
    zmq_msg_close(&zmq_broadcastData);
    return 0;

    Client dataSub = UNIVERSE_DATA__INIT;

}

int com_ReqClient(void *s_, uint64_t key, enum SDL_EventType tipo,  enum SDL_Scancode input){
    Client movReq = CLIENT__INIT;
    switch (tipo)
    {
    case SDL_KEYDOWN:
        switch (input)
        {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            movReq.input = 0;
            break;

        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            movReq.input = 1;
            break;
        
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            movReq.input = 2;
            break;
        
        default:
            movReq.input = 3;
            break;
        }
        break;
    
    default:
        switch (input)
        {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            movReq.input = 4;
            break;

        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            movReq.input = 5;
            break;
        
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            movReq.input = 6;
            break;
        
        default:
            movReq.input = 7;
            break;
        }
        break;
    }
    movReq.type = ENUM_TYPE__inputConfirmationRequest;
    movReq.key = key;

    size_t const ReqLen = client__get_packed_size(&movReq);
    uint8_t const *buffer = (uint8_t *)malloc(ReqLen);
    if (!buffer)
    {
        printf("Failed to maloc for movReq buffer.\n");
        return -1;
    }
    client__pack(&movReq, buffer);

    zmq_send(s_, buffer, ReqLen, 0);

    //Discard reply
    zmq_msg_t temp;
    zmq_msg_init(&temp);
    zmq_msg_recv(&temp, s_, 0);
    zmq_msg_close(&temp);

    free(buffer);
    return 0;

}

int com_QuitRecClient(void *s_, uint64_t key){
    Client QuitReq = CLIENT__INIT;
    QuitReq.type = ENUM_TYPE__disconnectRequest;
    QuitReq.key = key;

    size_t const ReqLen = client__get_packed_size(&QuitReq);

    uint8_t const *buffer = (uint8_t *)malloc(ReqLen);
    if (!buffer)
    {
        printf("Failed to maloc for QuitReq buffer.\n");
        return -1;
    }
    client__pack(&QuitReq, buffer);
    zmq_send(s_, buffer, ReqLen, 0);

    //Discard replty
    zmq_msg_t temp;
    zmq_msg_init(&temp);
    zmq_msg_recv(&temp, s_, 0);
    zmq_msg_close(&temp);

    free(buffer);
    return 0;
}