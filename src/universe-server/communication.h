#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdlib.h>

#include "universe-data.h"

#include <zmq.h> //zmq to send/receive
#include "../msgB.pb-c.h" //protobud for message structure

typedef struct ClientID {
    //ZMQ identity for ROUTER socket
    uint8_t identity[256];  //Store client's ZMQ identity
    size_t identity_size;   //Size of identity frame

    char ship_id;

} ClientID;


//Communication manager holding all ZMQ sockets
//Single context shared across all sockets
typedef struct CommunicationManager {
    void* context;
    
    //Client communication
    void* client_router_socket;        //ROUTER: handles client connect/disconnect/moves
    void* client_pub_socket;        //PUB: broadcasts game state to all clients
    ClientID* clients;
    int max_clients;                 //Maximum number of clients allowed
    int num_connected;               //Current number of connected clients
    
    //Dashboard communication  
    void* dashboard_rep_socket;     //REP: handles dashboard requests
} CommunicationManager;


//=== Initialization & Cleanup ===
CommunicationManager* CommunicationInit(GameState* state);
void CommunicationQuit(CommunicationManager** comm);


//=== Client Communication ===

//Process incoming client messages (connect, disconnect, moves)
//Uses zmq_poll to check for messages without blocking
//Returns: 1 if message processed, 0 if no message, -1 on error
int client_receive_and_process(CommunicationManager* comm);

//Send response to client (connection response or move acknowledgment)
//Uses ZMQ identity of last received message to route reply
int client_send_response(CommunicationManager* comm, ServerConnectMessage* msg);

//Broadcast game state to all connected clients via PUB socket
int client_broadcast_game_state(CommunicationManager* comm, UniverseData* data);


//=== Dashboard Communication ===

//Process incoming dashboard request
//Returns: 1 if message processed, 0 if no message, -1 on error
int dashboard_receive_and_process(CommunicationManager* comm);

//Send response to dashboard with simplified universe data
int dashboard_send_response(CommunicationManager* comm, SimplifiedUniverseData* data);


//=== Message Handling Helpers ===

//Deserialize received protobuf messages
ClientConnectMessage* deserialize_client_connect(void* data, size_t size);
Client* deserialize_client_move(void* data, size_t size);

//Serialize protobuf messages for sending
void* serialize_server_connect(ServerConnectMessage* msg, size_t* out_size);
void* serialize_universe_data(UniverseData* data, size_t* out_size);
void* serialize_simplified_data(SimplifiedUniverseData* data, size_t* out_size);

//Free serialized message buffers
void free_serialized_message(void* buffer);

#endif //COMMUNICATION_H