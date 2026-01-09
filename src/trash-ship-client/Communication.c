#include "Communication.h"


void *safe_zmq_ctx_new(gful_lifo **graceful_lifo){
    void *zmq_ctx = zmq_ctx_new();
    if(!zmq_ctx){
        printf("Error initializaing ZMQ_ctx: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }
    createContextDataforClosing((genericfunction *)zmq_close, zmq_ctx, graceful_lifo);
    return zmq_ctx;
}

void *safe_zmq_socket(void * zmq_ctx, int type_, gful_lifo **graceful_lifo){

    void *pReceive = zmq_socket(zmq_ctx, type_);
    if(!pReceive){
        printf("error initializing ZMQ_Socket: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }

    createContextDataforClosing((genericfunction *)zmq_close, pReceive, graceful_lifo);
    return pReceive;
}

void safe_zmq_bind(void *s_, const char *addr_, gful_lifo **graceful_lifo){

    int status = zmq_bind(s_, addr_);
    if(status){
        printf("Error initializing ZMQ_Bind: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }
}

void safe_zmq_connect(void *s_, const char *addr_, gful_lifo **graceful_lifo){

    int status = zmq_connect(s_, addr_);
    if(status){
        printf("Error initializing ZMQ_Connect: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }
}

int safe_zmq_send(  void *s_, const void *buf_, size_t len_, int flags_, 
                    gful_lifo **graceful_lifo){
    printf("Going to send Message.\n");
    int status = zmq_send(s_, buf_, len_, flags_);

    if(status == -1){
        printf("Error sending msg: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }
    return status;
}

int safe_zmq_recv(  void *s_, void *buf_, size_t len_, int flags_, 
                    gful_lifo **graceful_lifo){
    int status = zmq_recv(s_, buf_, len_, flags_);
    if(status == -1){
        printf("Error sending msg: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }
    return status;
}

int safe_zmq_recvmsg(void *s_, zmq_msg_t *msg_, int flags_, gful_lifo **graceful_lifo){
    int msg_len = zmq_recvmsg(s_, msg_ , flags_);
    if(msg_len == -1){
        printf("Error getting ZMQ message: %s\n", zmq_strerror(errno));
        closeContexts(*graceful_lifo);
        exit(1);
    }
    return msg_len;
}

int safe_zmq_msg_recv(void *s_, zmq_msg_t *recvdMsg, int flags_){
    zmq_msg_init(recvdMsg);

    int len = zmq_msg_recv(recvdMsg, s_, flags_);

    if(len == -1){
        zmq_msg_close(recvdMsg);
        return -1;
    }

    return 0;
}


int client_message_send(void * s_, ClientMessage sendMessage, gful_lifo **gracefull_lifo){

    //Prepare and send
        int msg_len = client_message__get_packed_size(&sendMessage);

        uint8_t *buffer = (uint8_t *)malloc(msg_len);
        if(!buffer)
        {
            printf("Failed to buffer for create message to send in client_message_send.\n");
            return -1;
        }

        createContextDataforClosing((genericfunction *)free, buffer, gracefull_lifo);

        client_message__pack(&sendMessage, buffer);

        int status = safe_zmq_send(s_, buffer, msg_len, 0, gracefull_lifo);

        closeSingleContext(gracefull_lifo);

        return status;
}

ServerMessage *zmq_msg_t_To_server_message(zmq_msg_t *zmqMsg){
    if (zmqMsg == NULL)
    {
        printf("Warning: pointer to zmqMsg in zmq_msg_t_To_server_message was NULL.\n");
        return NULL;
    }
    size_t len = zmq_msg_size(zmqMsg);
    uint8_t *buff = (uint8_t *)zmq_msg_data(zmqMsg);

    ServerMessage *serverRep = server_message__unpack(NULL, len, buff);

    if(zmq_msg_close(zmqMsg))
    {
        printf("Warning: Invalid message sent to zmq_msg_close at zmq_msg_t_To_server_message.\n");
    }

    return serverRep;
}

UniverseStateMessage *zmq_msg_t_To_UniverseStateMessage(zmq_msg_t *zmqMsg){
    if (zmqMsg == NULL)
    {
        printf("Warning: pointer to zmqMsg in zmq_msg_t_To_UniverseStateMessage was NULL.\n");
        return NULL;
    }
    size_t len = zmq_msg_size(zmqMsg);
    uint8_t *buff = (uint8_t *)zmq_msg_data(zmqMsg);

    UniverseStateMessage *serverSub = universe_state_message__unpack(NULL, len, buff);

    if(zmq_msg_close(zmqMsg))
    {
        printf("Warning: Invalid message sent to zmq_msg_close at zmq_msg_t_To_server_message.\n");
    }

    return serverSub;
}

void wrapper_server_message_free_unpacked(void *arg)
{
    free_unpacked_args *Data = (free_unpacked_args *)arg;
    server_message__free_unpacked(Data->message, Data->allocator);
}

/*
void initCntrlMsg(uint8_t *msg, uint8_t myID, ClientMessage *proto_msg){
    *msg |= myID;
    proto_msg->ch.data = msg;
    proto_msg->ch.len = 1;
}

void send_cntrl(void *s_, ClientMessage sendMessage, gful_lifo **gracefull_lifo){

    size_t size = client__get_packed_size(&sendMessage);
    uint8_t *buffer = (uint8_t *)malloc(size);
    createContextDataforClosing(free, buffer, gracefull_lifo);
    client__pack(&sendMessage,buffer);

    safe_zmq_send(s_, buffer, size, 0, gracefull_lifo);

    safe_zmq_recv(s_, buffer, size, 0, gracefull_lifo);

    closeSingleContext(gracefull_lifo);
}

ClientMessage *serverReceive(void *s_, zmq_msg_t *msg_, int flags_, gful_lifo **graceful_exit){
    int msg_len = safe_zmq_recvmsg(s_, msg_, flags_, graceful_exit);
    
    void *msg_data = zmq_msg_data(msg_);
    return client__unpack(NULL, msg_len, msg_data);
}

void    serverSend(uint8_t msg, void *pReceive, gful_lifo **graceful_lifo){
            
    ClientMessage protoMessageSend = CLIENT__INIT;
    protoMessageSend.ch.data = &msg;
    protoMessageSend.ch.len = 1;

    int size = client__get_packed_size(&protoMessageSend);
    uint8_t *buffer = malloc(size);
    createContextDataforClosing(free, buffer, graceful_lifo);
    client__pack(&protoMessageSend, buffer);
    
    safe_zmq_send(pReceive, buffer, size, 0, graceful_lifo);

    closeSingleContext(graceful_lifo);
}

uint8_t handleNewClient(Player **firstPlayer, int *nNewPlayers, GameState *game_state, gful_lifo *graceful_lifo){
    if(game_state->n_ships == 0){
        return newPLayerFirstPlace(firstPlayer, nNewPlayers, game_state, graceful_lifo);

    }else if(game_state->n_ships < game_state->n_planets){
        return newPlayerOtherPlace(*firstPlayer, game_state);

    }else{
        return newPlayerFull(nNewPlayers);
    }
}

    uint8_t newPLayerFirstPlace(Player **firstPlayer, int *nNewPlayers, GameState *game_state, gful_lifo *graceful_lifo){
        game_state->n_ships++;
        (*nNewPlayers)--;
        game_state->ships[0].is_active = 1;
        uint8_t newID = (uint8_t)(rand() % TOTALCOMBINATIONS);
        *firstPlayer = malloc(sizeof(Player));
        if(!firstPlayer){
            printf("Failed at malloc.\n");
            closeContexts(graceful_lifo);
            exit(1);
        }
        **firstPlayer = (Player){newID, game_state -> n_ships, NULL};
        return newID << 3;
    }

    uint8_t newPlayerOtherPlace(Player *firstPlayer, GameState *game_state){
        //Find unused iD
        uint8_t newID = (uint8_t)(rand() % TOTALCOMBINATIONS);
        Player * currPlayer;
        for(currPlayer = firstPlayer; currPlayer; newID = (uint8_t)(rand() % TOTALCOMBINATIONS)){
            currPlayer = firstPlayer;
            while(currPlayer){
                if(!currPlayer) break;
                currPlayer = currPlayer->nextPlayer;
            }
        }
        //Update number of players
        game_state->n_ships++;
        //Get unused ship number
        int spaceshipNum;
        for(spaceshipNum = 0; game_state->ships[spaceshipNum].is_active == 1; spaceshipNum++);
        //Update ship status
        game_state->ships[spaceshipNum].is_active = 1;
        //Create new player
        Player *newPlayer = malloc(sizeof(Player));
        *newPlayer = (Player){newID, spaceshipNum+1, NULL};
        //Place new player
        for(currPlayer = firstPlayer; currPlayer->nextPlayer != NULL; currPlayer = currPlayer->nextPlayer);
        currPlayer->nextPlayer = newPlayer;
        return newID << 3;
    }

    uint8_t newPlayerFull(int *nNewPlayers){
        (*nNewPlayers)--;
        return MAXPLAYERS;
    }

uint8_t handleClientExit(Player **firstPlayer, uint8_t msgSenderID, int *nNewPlayers, GameState *game_state){
    //Remove Player entity with corresponding id from player list.
        //Verificar que quem mandou a mensagem pertence à player list,
        //se pertencer guardar o pointer para o player
    Player *currPlayer = *firstPlayer;
    while(currPlayer != NULL){
        if(currPlayer->ID == msgSenderID) break;
        currPlayer = currPlayer->nextPlayer;
    }
        //Atualizar a lista de player para não conter o ex-player e libertar a menória por ele ocupada
    if (currPlayer != NULL){
        Player *temp = currPlayer;
        if(currPlayer == *firstPlayer){
            *firstPlayer = (*firstPlayer)->nextPlayer;
        }else{
            for(currPlayer = *firstPlayer; currPlayer->nextPlayer->ID != msgSenderID; currPlayer = currPlayer->nextPlayer);
            currPlayer->nextPlayer = temp->nextPlayer;
        }
        game_state->ships[temp->shipID - 1].is_active = 0;
        free(temp);
        game_state->n_ships--;
        return SUCCESS;
    } else {
        //Message sender was not in player list
        return FAIL;
        
    }
    //Add temporary players so this loop does't skip last attended client 
    //(note: if msg wasnt from client it works because it ignores the fake 
    //user and if a user was destroyed it doesn't shorten the get all client messages loop)
    nNewPlayers++;

                    
}

uint8_t handleClientStill(Player *firstPlayer, uint8_t msgSenderID, GameState *game_state, int *nNewPlayers){
    //Colocar na informação da spaceship speed 0
        //Verificar que quem mandou a mensagem pertence à player list,
        //se pertencer guardar o pointer para o player
    Player *currPlayer = firstPlayer;
    while(currPlayer != NULL){
        if(currPlayer->ID == msgSenderID) break;
        currPlayer = currPlayer->nextPlayer;
    }
    if(currPlayer)
    {
        int spaceshipNum = currPlayer->shipID;
        game_state->ships[spaceshipNum-1].direction = INVALID_DIRECTION;
        return SUCCESS;
    }else{
        //Player Id doesn't exist
        nNewPlayers++;
        return FAIL;
    }                    
}

uint8_t handleClientMove(Player *firstPlayer, uint8_t msgSenderID, GameState *game_state, int *nNewPlayers, uint8_t msg){

    //Colocar na informação da spaceship speed e angle
        //Verificar que quem mandou a mensagem pertence à player list,
        //se pertencer guardar o pointer para o player
    Player *currPlayer = firstPlayer;
    while(currPlayer != NULL){
        if(currPlayer->ID == msgSenderID) break;
        currPlayer = currPlayer->nextPlayer;
    }
    if(currPlayer)
    {
        int spaceshipNum = currPlayer->shipID;
        switch (msg & CODEMASK)
        {
        case MYUP:
            game_state->ships[spaceshipNum-1].direction = UP;
            break;

        case MYLEFT:
            game_state->ships[spaceshipNum-1].direction = LEFT;
            break;

        case MYDOWN:
            game_state->ships[spaceshipNum-1].direction = DOWN;
            break;

        default:
            game_state->ships[spaceshipNum-1].direction = RIGHT;
            break;
        }
        return SUCCESS;
    }else{
        //Player Id doesn't exist
        (*nNewPlayers)++;
        return FAIL;
    }       
}
*/
