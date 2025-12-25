#include "communication.h"
#include "universe-data.h"

#include "../msgB.pb-c.h"
#include <zmq.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define CLIENT_CONNECT CLIENT_CONNECT_MESSAGE_TYPE__CONNECT
#define CLIENT_DISCONNECT CLIENT_CONNECT_MESSAGE_TYPE__DISCONNECT

#define SERVER_CONNECT_OK SERVER_CONNECT_MESSAGE_TYPE__OK
#define SERVER_CONNECT_ERROR SERVER_CONNECT_MESSAGE_TYPE__ERROR

CommunicationManager* CommunicationInit(GameState* state) {

    int max_clients = state->n_ships;

    CommunicationManager* comm = malloc(sizeof(CommunicationManager));
    if (comm == NULL) {
        return NULL;
    }

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
    
    //Mark all slots as empty (ship_id = 0 means empty)
    for (int i = 0; i < max_clients; i++) {
        comm->clients[i].ship_id = -1;
        comm->clients[i].connection_id = -1;
    }

    //initialize client sockets

    //create a string for the ports
    char router_port_str[16];
    char pub_port_str[16];

    snprintf(router_port_str, sizeof(router_port_str), "tcp://*:%d", state->router_port);
    snprintf(pub_port_str, sizeof(pub_port_str), "tcp://*:%d", state->pub_port);

    comm->client_router_socket = zmq_socket(comm->context, ZMQ_REP);
    zmq_bind(comm->client_router_socket, router_port_str);

    comm->client_pub_socket = zmq_socket(comm->context, ZMQ_PUB);
    zmq_bind(comm->client_pub_socket, pub_port_str);

    //initialize dashboard socket
    comm->dashboard_rep_socket = zmq_socket(comm->context, ZMQ_REP);
    
    if (comm->client_router_socket == NULL || comm->client_pub_socket == NULL || comm->dashboard_rep_socket == NULL) {
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
    if ((*comm)->client_router_socket != NULL) {
        zmq_close((*comm)->client_router_socket);
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
        if (comm->clients[i].ship_id == -1) {
            slot = i;
            break;
        }
    }

    //create a message (we always send a response)
    ServerConnectMessage response = SERVER_CONNECT_MESSAGE__INIT;
    ServerConnectMessageType msg_type;
    char ship_id[2];

    if (slot == -1) {
        //error response
        msg_type = SERVER_CONNECT_ERROR;
        ship_id[0] = '0';
        ship_id[1] = '\0';
    } else {
        //communication manager updates
        comm->num_connected++;
        comm->clients[slot].ship_id = 'A' + slot;  //Assign ship ID as name
        comm->clients[slot].connection_id = slot;

        //prepare success response
        msg_type = SERVER_CONNECT_OK;
        ship_id[0] = comm->clients[slot].ship_id;
        ship_id[1] = '\0';

    }

    //send message along with assigned ship ID (ID = '0' if error)
    response.id = strdup(ship_id);
    response.msg_type = msg_type;

    //serialize protobuf message into zmq message and send
    int msg_len = server_connect_message__get_packed_size(&response);
    uint8_t* msg_buf = (uint8_t*)malloc(msg_len);
    server_connect_message__pack(&response, msg_buf);
    zmq_send(comm->client_router_socket, msg_buf, msg_len, 0);

    free(msg_buf);
    return;
}

void _ProcessClientDisconnect(CommunicationManager* comm) {
    //process a disconnection request from a client
    (void)comm;
}

int ReceiveClientConnection(CommunicationManager* comm) {
    if (comm == NULL || comm->client_router_socket == NULL) {
        return -1;
    }

    zmq_msg_t msg;
    zmq_msg_init(&msg);

    //client message
    ClientConnectMessage* connect_msg;
    
    //receive message
    int msg_len = zmq_recvmsg(comm->client_router_socket, &msg, 0);

    //unpack contents
    void* msg_data = zmq_msg_data(&msg);
    connect_msg = client_connect_message__unpack(NULL, msg_len, msg_data);


    int return_value = -1;
    //process message
    if (connect_msg->msg_type == CLIENT_CONNECT) {
        printf("CLIENT CONNECTED\n");
        _ProcessClientConnect(comm);
        return_value = 1;
    } else if (connect_msg->msg_type == CLIENT_DISCONNECT) {
        printf("CLIENT DISCONNECTED\n");
        _ProcessClientDisconnect(comm);
        return_value = 1;
    } else {
        //unknown message type
        return_value = -1;
    }

    client_connect_message__free_unpacked(connect_msg, NULL);
    zmq_msg_close(&msg);
    return return_value;
}
