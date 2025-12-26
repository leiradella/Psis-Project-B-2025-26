#include "../msgB.pb-c.h"
#include <zmq.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char* argv[]) {

    //create zmq context and socket
    void* ctx = zmq_ctx_new();

    void* socket = zmq_socket(ctx, ZMQ_REQ);

    //get port
    char port[32] = "tcp://localhost:6767";

    //connect to the port
    zmq_connect(socket, port);
    printf("connected\n");

    ClientMessage msg = CLIENT_MESSAGE__INIT;

    msg.msg_type = CLIENT_MESSAGE_TYPE__CONNECT;

    int msg_len = client_message__get_packed_size(&msg);
    uint8_t* msg_buf = malloc(msg_len);

    client_message__pack(&msg, msg_buf);

    zmq_send(socket, msg_buf, msg_len, 0);

    free(msg_buf);

    //receive response
    uint8_t recv_buf[256];
    int recv_len = zmq_recv(socket, recv_buf, sizeof(recv_buf), 0);
    
    ServerMessage* response = server_message__unpack(NULL, recv_len, recv_buf);
    printf("Received response: type=%d, id=%s\n", response->msg_type, response->id);

    //store ID for later use
    char* id = strdup(response->id);

    server_message__free_unpacked(response, NULL);



    //now send a disconnect message
    ClientMessage disc_msg = CLIENT_MESSAGE__INIT;

    disc_msg.msg_type = CLIENT_MESSAGE_TYPE__DISCONNECT;
    disc_msg.id = strdup(id);

    int disc_msg_len = client_message__get_packed_size(&disc_msg);
    uint8_t* disc_msg_buf = (uint8_t*)malloc(disc_msg_len);

    client_message__pack(&disc_msg, disc_msg_buf);
    zmq_send(socket, disc_msg_buf, disc_msg_len, 0);
    free(disc_msg.id);
    free(disc_msg_buf);
    free(id);

    //receive response
    recv_len = zmq_recv(socket, recv_buf, sizeof(recv_buf), 0);
    response = server_message__unpack(NULL, recv_len, recv_buf);
    printf("Received disconnect response: type=%d, id=%s\n", response->msg_type, response->id);
    server_message__free_unpacked(response, NULL);

    zmq_close(socket);
    zmq_ctx_destroy(ctx);
    
    return 0;
}