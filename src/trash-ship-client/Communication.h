#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include <zmq.h>
//#include "../universe-data.h"
#include "graceful-exit.h"
#include "../msgB.pb-c.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

//Descriptors of message content

#define GETID   248         //For server

#define MAXPLAYERS  248     //For client
#define SUCCESS     1       
#define FAIL        0 

#define ISMOVE  0
#define ISDC    7
#define MYSTILL 4

#define MYLEFT  1
#define MYUP    0
#define MYDOWN  2
#define MYRIGHT 3

#define IDMASK  248
#define CODEMASK 7

#define LASTSHIPID 19

//Other constants
#define TOTALCOMBINATIONS 31
#define SPACESHIPSPEED 10
#define ANGLEUP 0
#define ANGLELEFT PI / 2
#define ANGLEDOWN PI
#define ANGLERIGHT 3 * PI / 2

#define MSGLEN 3

typedef struct zmq_close_args{
    void *s_;
    const char *addr;
} zmq_disconnectArgs;

//The safe class function wrap their namesakes with their respective error
//handling and with graceful exit data.
void *safe_zmq_ctx_new(gful_lifo **graceful_lifo);

void *safe_zmq_socket(void *, int type_, gful_lifo **graceful_lifo);

void safe_zmq_bind(void *s_, const char *addr_, gful_lifo **graceful_lifo);

void safe_zmq_connect(void *s_, const char *addr_, gful_lifo **graceful_lifo);

int safe_zmq_send(void *s_, const void *buf_, size_t len_, int flags_, gful_lifo **graceful_lifo);

int safe_zmq_recv(void *s_, void *buf_, size_t len_, int flags_, gful_lifo **graceful_lifo);

int safe_zmq_recvmsg(void *s_, zmq_msg_t *msg_, int flags_, gful_lifo **graceful_exit);

int safe_zmq_msg_recv(void *s_, zmq_msg_t *recvdMsg, int flags_);

//Client
int client_message_send(void *s_, ClientMessage sendMessage, gful_lifo **gracefull_lifo);

ServerMessage *zmq_msg_t_To_server_message(zmq_msg_t *zmqMsg);

UniverseStateMessage *zmq_msg_t_To_UniverseStateMessage(zmq_msg_t *zmqMsg);


void initCntrlMsg(uint8_t *msg, uint8_t myID, ClientMessage *proto_msg);

void send_cntrl(void *s_, ClientMessage sendMessage, gful_lifo **gracefull_lifo);
/*
//Server
ClientMessage *serverReceive(void *s_, zmq_msg_t *msg_, int flags_, gful_lifo **graceful_exit);
void    serverSend(uint8_t msg, void *pReceive, gful_lifo **graceful_lifo);
*/
/*
uint8_t handleNewClient(Player **firstPlayer, int *nNewPlayers, GameState *game_state, gful_lifo *graceful_lifo);
    uint8_t newPLayerFirstPlace(Player **firstPlayer, int *nNewPlayers, GameState *game_state, gful_lifo *graceful_lifo);
    uint8_t newPlayerOtherPlace(Player *firstPlayer, GameState *game_state);
    uint8_t newPlayerFull(int *nNewPlayers);

uint8_t handleClientExit(Player **firstPlayer, uint8_t msgSenderID, int *nNewPlayers, GameState *game_state);

uint8_t handleClientStill(Player *firstPlayer, uint8_t msgSenderID, GameState *game_state, int *nNewPlayers);

uint8_t handleClientMove(Player *firstPlayer, uint8_t msgSenderID, GameState *game_state, int *nNewPlayers, uint8_t msg);
*/
#endif // MYLIB_H
