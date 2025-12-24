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
        comm->clients[i].ship_id = 0;
        comm->clients[i].identity_size = 0;
    }

    //initialize client sockets

    //create a string for the ports
    char router_port_str[16];
    char pub_port_str[16];

    snprintf(router_port_str, sizeof(router_port_str), "tcp://*:%d", state->router_port);
    snprintf(pub_port_str, sizeof(pub_port_str), "tcp://*:%d", state->pub_port);

    comm->client_router_socket = zmq_socket(comm->context, ZMQ_ROUTER);
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

int client_receive_and_process(CommunicationManager* comm) {
    if (comm == NULL || comm->client_router_socket == NULL) {
        return -1;
    }

    //ROUTER receives multi-part message: [identity][empty][data]
    zmq_msg_t identity_msg, empty_msg, data_msg;
    
    //Receive identity frame
    zmq_msg_init(&identity_msg);
    int id_len = zmq_msg_recv(&identity_msg, comm->client_router_socket, 0);
    if (id_len == -1) {
        zmq_msg_close(&identity_msg);
        return -1;
    }
    
    //Receive empty delimiter frame
    zmq_msg_init(&empty_msg);
    int empty_len = zmq_msg_recv(&empty_msg, comm->client_router_socket, 0);
    if (empty_len == -1) {
        zmq_msg_close(&identity_msg);
        zmq_msg_close(&empty_msg);
        return -1;
    }
    
    //Receive actual data frame
    zmq_msg_init(&data_msg);
    int data_len = zmq_msg_recv(&data_msg, comm->client_router_socket, 0);
    if (data_len == -1) {
        zmq_msg_close(&identity_msg);
        zmq_msg_close(&empty_msg);
        zmq_msg_close(&data_msg);
        return -1;
    }
    
    //Extract identity
    void* identity_data = zmq_msg_data(&identity_msg);
    size_t identity_size = zmq_msg_size(&identity_msg);
    
    //Extract message data
    void* msg_data = zmq_msg_data(&data_msg);
    ClientConnectMessage* connect_msg = client_connect_message__unpack(NULL, data_len, msg_data);
    
    if (connect_msg == NULL) {
        zmq_msg_close(&identity_msg);
        zmq_msg_close(&empty_msg);
        zmq_msg_close(&data_msg);
        return -1;
    }

    //Process message based on type
    if (connect_msg->msg_type == CLIENT_CONNECT) {
        //Find available slot and assign ID
        int slot = -1;
        for (int i = 0; i < comm->max_clients; i++) {
            if (comm->clients[i].ship_id == 0) {
                slot = i;
                break;
            }
        }
        
        ServerConnectMessage response_msg = SERVER_CONNECT_MESSAGE__INIT;

        if (slot == -1) {
            printf("Server full\n");

            response_msg.msg_type = SERVER_CONNECT_ERROR;
            char ship_id_str[2] = {'0', '\0'};
            response_msg.id = strdup(ship_id_str);

        } else {
            //Store client identity and assign ship_id
            comm->clients[slot].ship_id = 'A' + slot;
            comm->clients[slot].identity_size = identity_size;
            memcpy(comm->clients[slot].identity, identity_data, identity_size);
            comm->num_connected++;
            
            printf("Client connected: assigned ID '%c'\n", comm->clients[slot].ship_id);

            response_msg.msg_type = SERVER_CONNECT_OK;
            char ship_id_str[2] = {comm->clients[slot].ship_id, '\0'};
            response_msg.id = strdup(ship_id_str);
        }

        client_send_response(comm, &response_msg, (uint8_t*)identity_data, identity_size);


    } else if (connect_msg->msg_type == CLIENT_DISCONNECT) {
        //Find client by identity
        int slot = -1;
        for (int i = 0; i < comm->max_clients; i++) {
            if (comm->clients[i].identity_size == identity_size &&
                memcmp(comm->clients[i].identity, identity_data, identity_size) == 0) {
                slot = i;
                break;
            }
        }
        
        if (slot != -1) {
            printf("Client '%c' disconnected\n", comm->clients[slot].ship_id);
            comm->clients[slot].ship_id = 0;
            comm->clients[slot].identity_size = 0;
            comm->num_connected--;
        }

    } else {
        printf("Unknown message type\n");
    }
    
    //Cleanup
    client_connect_message__free_unpacked(connect_msg, NULL);
    zmq_msg_close(&identity_msg);
    zmq_msg_close(&empty_msg);
    zmq_msg_close(&data_msg);
    
    return 1;
}

int client_send_response(CommunicationManager* comm, ServerConnectMessage* msg, uint8_t* identity, size_t identity_size) {
    if (comm == NULL || comm->client_router_socket == NULL || msg == NULL) {
        return -1;
    }

    //serialize message
    size_t msg_size = server_connect_message__get_packed_size(msg);
    uint8_t* buffer = malloc(msg_size);
    if (buffer == NULL) {
        return -1;
    }
    server_connect_message__pack(msg, buffer);

    //send multi-part message: [identity][empty][data]
    zmq_msg_t identity_msg, empty_msg, data_msg;

    //identity frame
    zmq_msg_init_size(&identity_msg, identity_size);
    memcpy(zmq_msg_data(&identity_msg), identity, identity_size);
    zmq_msg_send(&identity_msg, comm->client_router_socket, ZMQ_SNDMORE);
    zmq_msg_close(&identity_msg);

    //empty delimiter frame
    zmq_msg_init_size(&empty_msg, 0);
    zmq_msg_send(&empty_msg, comm->client_router_socket, ZMQ_SNDMORE);
    zmq_msg_close(&empty_msg);

    //data frame
    zmq_msg_init_size(&data_msg, msg_size);
    memcpy(zmq_msg_data(&data_msg), buffer, msg_size);
    zmq_msg_send(&data_msg, comm->client_router_socket, 0);
    zmq_msg_close(&data_msg);
    
    free(msg->id);
    free(buffer);

    return 0;
}