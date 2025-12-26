#include "communication.h"
#include "universe-data.h"

#include "../msgB.pb-c.h"
#include <zmq.h>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>


#define CLIENT_CONNECT CLIENT_MESSAGE_TYPE__CONNECT
#define CLIENT_DISCONNECT CLIENT_MESSAGE_TYPE__DISCONNECT
#define CLIENT_MOVE CLIENT_MESSAGE_TYPE__MOVE

#define SERVER_CONNECT_OK SERVER_MESSAGE_TYPE__OK
#define SERVER_CONNECT_ERROR SERVER_MESSAGE_TYPE__ERROR

CommunicationManager* CommunicationInit(GameState* state) {

    int max_clients = state->n_ships;

    CommunicationManager* comm = malloc(sizeof(CommunicationManager));
    if (comm == NULL) {
        return NULL;
    }

    //add gamestate reference
    comm->game_state = state;

    //initialize ZMQ context
    comm->context = zmq_ctx_new();
    if (comm->context == NULL) {
        free(comm);
        return NULL;
    }

    //allocate memory for clients array
    comm->clients = (ClientID*)malloc(sizeof(ClientID) * max_clients);
    if (comm->clients == NULL) {
        zmq_ctx_destroy(comm->context);
        free(comm);
        return NULL;
    }
    
    //Initialize client tracking
    comm->max_clients = max_clients;
    comm->num_connected = 0;
    
    //Mark all slots as empty (ship_id = -1 means empty)
    for (int i = 0; i < max_clients; i++) {
        comm->clients[i].ship_index = -1;
        comm->clients[i].connection_id = NULL;
    }

    //initialize client sockets

    //create a string for the ports
    char rep_port_str[16];
    char pub_port_str[16];

    snprintf(rep_port_str, sizeof(rep_port_str), "tcp://*:%d", state->rep_port);
    snprintf(pub_port_str, sizeof(pub_port_str), "tcp://*:%d", state->pub_port);

    comm->client_rep_socket = zmq_socket(comm->context, ZMQ_REP);
    zmq_bind(comm->client_rep_socket, rep_port_str);

    comm->client_pub_socket = zmq_socket(comm->context, ZMQ_PUB);
    zmq_bind(comm->client_pub_socket, pub_port_str);

    //initialize dashboard socket
    comm->dashboard_rep_socket = zmq_socket(comm->context, ZMQ_REP);
    
    if (comm->client_rep_socket == NULL || comm->client_pub_socket == NULL || comm->dashboard_rep_socket == NULL) {
        free(comm->clients);
        zmq_ctx_destroy(comm->context);
        free(comm);
        return NULL;
    }

    return comm;
}

void CommunicationQuit(CommunicationManager** comm) {
    if (comm == NULL || *comm == NULL) {
        return;
    }

    //close ZMQ sockets
    if ((*comm)->client_rep_socket != NULL) {
        zmq_close((*comm)->client_rep_socket);
    }
    if ((*comm)->client_pub_socket != NULL) {
        zmq_close((*comm)->client_pub_socket);
    }
    if ((*comm)->dashboard_rep_socket != NULL) {
        zmq_close((*comm)->dashboard_rep_socket);
    }

    //destroy ZMQ context
    if ((*comm)->context != NULL) {
        zmq_ctx_destroy((*comm)->context);
    }

    //free clients array
    if ((*comm)->clients != NULL) {
        free((*comm)->clients);
    }

    //free CommunicationManager struct
    free(*comm);
    *comm = NULL;
}

void _ProcessClientConnect(CommunicationManager* comm) {
    //process a connection request from a client
    //check for an empty slot
    int slot = -1;
    for (int i=0; i<comm->max_clients; i++) {
        if (comm->clients[i].ship_index == -1) {
            slot = i;
            break;
        }
    }

    //create a message (we always send a response)
    ServerMessage response = SERVER_MESSAGE__INIT;
   
    //message contents
    ServerMessageType msg_type;
    char id[33];

    if (slot == -1) {
        //error response
        msg_type = SERVER_CONNECT_ERROR;
        
        //invalid ID for error
        id[0] = '\0';
    } else {
        //communication manager updates
        comm->num_connected++;

        //slot is an empty index for ship
        comm->clients[slot].ship_index = slot;
        
        //create a random connection ID
        static const char *hex = "0123456789abcdef";
        for (int i=0; i<16; i++) {
            unsigned int r = (unsigned int)rand();
            r ^= (unsigned int)rand() << 8;

            unsigned int byte = r & 0xFF;
            id[i*2] = hex[(byte >> 4) & 0xF];
            id[i*2 +1] = hex[byte & 0xF];
        }
        id[32] = '\0';

        //assign ID to manager
        comm->clients[slot].connection_id = strdup(id);

        //enable ship in gamestate (lock because is_active is accessed by other threads)
        pthread_mutex_lock(&comm->game_state->mutex_enable);
        comm->game_state->ships[slot].enabled = 1;
        pthread_mutex_unlock(&comm->game_state->mutex_enable);
        //now when the client sends a move message with this id, we move ship at ship_index
        //when a new client connects they are guaranteed to not get the same ship index

        //prepare success response
        msg_type = SERVER_CONNECT_OK;
    }

    //send message along with assigned connection ID
    response.id = strdup(id);
    response.msg_type = msg_type;

    //serialize protobuf message into zmq message and send
    int msg_len = server_message__get_packed_size(&response);
    uint8_t* msg_buf = (uint8_t*)malloc(msg_len);
    server_message__pack(&response, msg_buf);
    zmq_send(comm->client_rep_socket, msg_buf, msg_len, 0);

    free(response.id);
    free(msg_buf);
    return;
}

void _ProcessClientDisconnect(CommunicationManager* comm, char* client_id) {
    //process a disconnection request from a client

    //find the slot with the given ship_id
    int slot = -1;
    for (int i=0; i<comm->max_clients; i++) {
        char* id = comm->clients[i].connection_id;

        if (strcasecmp(id, client_id) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        //unknown ship_id, ignore
        return;
    } else {
        //communication manager updates
        comm->num_connected--;
        comm->clients[slot].ship_index = -1;
        free(comm->clients[slot].connection_id);
        comm->clients[slot].connection_id = NULL;

        //disable ship in gamestate (lock because is_active is accessed by other threads)
        pthread_mutex_lock(&comm->game_state->mutex_enable);
        comm->game_state->ships[slot].enabled = 0;
        pthread_mutex_unlock(&comm->game_state->mutex_enable);

        //need to send response
        ServerMessageType msg_type = SERVER_CONNECT_OK;
        char id[33];
        id[0] = '\0';

        //send message along with assigned connection ID
        ServerMessage response = SERVER_MESSAGE__INIT;
        response.id = strdup(id);
        response.msg_type = msg_type;

        //serialize protobuf message into zmq message and send
        int msg_len = server_message__get_packed_size(&response);
        uint8_t* msg_buf = (uint8_t*)malloc(msg_len);
        server_message__pack(&response, msg_buf);
        zmq_send(comm->client_rep_socket, msg_buf, msg_len, 0);

        free(response.id);
        free(msg_buf);
        return;
    }
}

void _ProcessClientMove(CommunicationManager* comm, char* client_id, protobuf_c_boolean* keys) {

    //find the slot with the client_id to get the ship index
    int slot = -1;
    for (int i=0; i<comm->max_clients; i++) {
        char* id = comm->clients[i].connection_id;
        if (strcasecmp(id, client_id) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        //unknown client_id, ignore
        return;
    } else {

        //decypher keys
        //lock
        pthread_mutex_lock(&comm->game_state->mutex_keys);

        //first 4 keydowns: w, a, s, d
        if (keys[0]) comm->game_state->ships[slot].thrust = 0.10f;
        if (keys[1]) comm->game_state->ships[slot].rotation = -5.0f;
        if (keys[2]) comm->game_state->ships[slot].thrust = 0.0f;
        if (keys[3]) comm->game_state->ships[slot].rotation = 5.0f;

        //4 keyups: w, a, s, d
        if (keys[4]) comm->game_state->ships[slot].thrust = 0.0f;
        if (keys[5]) comm->game_state->ships[slot].rotation = 0.0f;
        if (keys[6]) comm->game_state->ships[slot].thrust = 0.0f;
        if (keys[7]) comm->game_state->ships[slot].rotation = 0.0f;

        //unlock
        pthread_mutex_unlock(&comm->game_state->mutex_keys);
    }

    //send response
    ServerMessage response = SERVER_MESSAGE__INIT;
    response.id = strdup(client_id);
    response.msg_type = SERVER_CONNECT_OK;

    //serialize protobuf message into zmq message and send
    int msg_len = server_message__get_packed_size(&response);
    uint8_t* msg_buf = (uint8_t*)malloc(msg_len);
    server_message__pack(&response, msg_buf);
    zmq_send(comm->client_rep_socket, msg_buf, msg_len, 0);

    free(response.id);
    free(msg_buf);
    return;
}

int ReceiveClientMessage(CommunicationManager* comm) {
    if (comm == NULL || comm->client_rep_socket == NULL) {
        return -1;
    }

    zmq_msg_t msg;
    zmq_msg_init(&msg);

    //client message
    ClientMessage* client_msg;
    
    //receive message
    int msg_len = zmq_recvmsg(comm->client_rep_socket, &msg, 0);

    //unpack contents
    void* msg_data = zmq_msg_data(&msg);
    client_msg = client_message__unpack(NULL, msg_len, msg_data);


    int return_value = -1;
    //process message
    if (client_msg->msg_type == CLIENT_CONNECT) {
        printf("CLIENT CONNECTED\n");
        _ProcessClientConnect(comm);
        return_value = 1;
    } else if (client_msg->msg_type == CLIENT_DISCONNECT) {
        printf("CLIENT DISCONNECTED\n");
        _ProcessClientDisconnect(comm, client_msg->id);
        return_value = 1;
    } else if (client_msg->msg_type == CLIENT_MOVE) {
        //process move message
        printf("CLIENT MOVE MESSAGE RECEIVED\n");

        //transform the keys into an array of booleans
        protobuf_c_boolean keys[8];
        keys[0] = client_msg->wkeydown;
        keys[1] = client_msg->akeydown;
        keys[2] = client_msg->skeydown;
        keys[3] = client_msg->dkeydown;
        keys[4] = client_msg->wkeyup;
        keys[5] = client_msg->akeyup;
        keys[6] = client_msg->skeyup;
        keys[7] = client_msg->dkeyup;

        _ProcessClientMove(comm, client_msg->id, keys);
        return_value = 1;
    } else {
        //unknown message type
        return_value = -1;
    }

    client_message__free_unpacked(client_msg, NULL);
    zmq_msg_close(&msg);
    return return_value;
}
