#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdlib.h>

#include "universe-data.h"

#include <zmq.h> //zmq to send/receive
#include "../msgB.pb-c.h" //protobud for message structure

typedef struct ClientID {
    int ship_index;     //index of the ship assigned to this client
    char* connection_id;  //Unique connection ID for this client
} ClientID;

//communication manager holding all ZMQ sockets
//single context shared across all sockets
typedef struct CommunicationManager {
    
    //gamestate to enable/disable ships on connect/disconnect
    GameState* game_state;

    //flag for thread termination
    int terminate_thread;
    pthread_mutex_t mutex_terminate; //need a mutex because thread reads it and main thread writes it
    
    void* context;

    //====Client communication======
    //sockets
    void* client_rep_socket;        //REP: handles client connect/disconnect/moves
    void* client_pub_socket;           //PUB: broadcasts game state to all clients
    
    //client tracking
    ClientID* clients;               //array of connected clients
    int max_clients;                 //maximum number of clients allowed
    int num_connected;               //current number of connected clients

    
    //======dashboard communication======
    void* dashboard_rep_socket;     //REP: handles dashboard requests



} CommunicationManager;


//=== Initialization & Cleanup ===
CommunicationManager* CommunicationInit(GameState* state);
void CommunicationQuit(CommunicationManager** comm);


//=== Client Communication ===

//Process incoming client messages (connect, disconnect, moves)
//Uses zmq_poll to check for messages without blocking
//Returns: 1 if message processed, 0 if no message, -1 on error
int ReceiveClientMessage(CommunicationManager* comm);

//Send response to client (connection response or move acknowledgment)
//Uses ZMQ identity of last received message to route reply
int client_send_response(CommunicationManager* comm, ServerMessage* msg, uint8_t* identity, size_t identity_size);

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
ClientMessage* deserialize_client_connect(void* data, size_t size);
Client* deserialize_client_move(void* data, size_t size);

//Serialize protobuf messages for sending
void* serialize_server_connect(ServerMessage* msg, size_t* out_size);
void* serialize_universe_data(UniverseData* data, size_t* out_size);
void* serialize_simplified_data(SimplifiedUniverseData* data, size_t* out_size);

//Free serialized message buffers
void free_serialized_message(void* buffer);

#endif //COMMUNICATION_H