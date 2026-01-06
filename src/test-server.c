#include <zmq.h>
#include "./Common/display.h"
#include "./Common/msgB.pb-c.h"
#include "./Common/Communication.h"

int main(int argc, char *argv[])
{
//Prepare GL
    gful_lifo *programContext = GFUL_INIT;
//Prepare ZMQ
    void *zmq_ctx = safe_zmq_ctx_new(&programContext);
    void *connectioSocket = safe_zmq_socket(zmq_ctx, ZMQ_REP, &programContext);
    safe_zmq_bind(connectioSocket, "tcp://*:5556", &programContext);

//send Message Loop
    int end = 0;
    int sendState = 0;
    while (!end)
    {
        scanf(NULL);

        sendState = safe_zmq_send(connectioSocket, );
        safe_zmq_recvmsg(connectioSocket);
    }
    
    closeContexts(programContext);
    return 0;
}