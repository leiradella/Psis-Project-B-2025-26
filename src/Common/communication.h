#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdlib.h>

#include "universe-data.h"

#include <zmq.h> //zmq to send/receive
#include "../msgB.pb-c.h" //protobud for message structure

#define CLIENT_TIMEOUT_SEC 30 //time in seconds before a client is considered disconnected

typedef struct ClientID {
    int ship_index;     //index of the ship assigned to this client
    char* connection_id;  //Unique connection ID for this client
    Uint32 last_active_time; //last time we received a message from this client
} ClientID;

//communication manager holding all ZMQ sockets
//single context shared across all sockets
typedef struct CommunicationManager {
    
    //gamestate to enable/disable ships on connect/disconnect
    GameState* game_state;

    //gamestate snapshot for publishing
    GameStateSnapshot* snapshot;

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
CommunicationManager* CommunicationInit(GameState* state, GameStateSnapshot* snapshot);
void CommunicationQuit(CommunicationManager** comm);


//=== Client Communication ===
//receives a client message and processes it
//1 on success, 0 on no message, -1 on error
int ReceiveClientMessage(CommunicationManager* comm);

//checks for client timeouts and disconnects them if timed out
void CheckClientTimeouts(CommunicationManager* comm);

//=== Universe State Publishing ===
void SendUniverseState(CommunicationManager* comm);

//=== Dashboard Communication ===
void SendDashboardUpdate(CommunicationManager* comm);

#endif //COMMUNICATION_H