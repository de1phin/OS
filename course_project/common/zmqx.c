#include "zmqx.h"
#include "zmq.h"
#include "string.h"
#include "stdlib.h"
#include "assert.h"
#include "stdio.h"

zmqx zmqx_new_listener(int port) {
    zmqx zmqx;
    zmqx.context = zmq_ctx_new();
    zmqx.connection = zmq_socket(zmqx.context, ZMQ_REP);
    char* addr = malloc(30);
    sprintf(addr, "tcp://*:%d", port);
    int rc = zmq_bind(zmqx.connection, addr);
    assert(rc == 0);
    return zmqx;
}

zmqx zmqx_new_writer(int port) {
    zmqx zmqx;
    zmqx.context = zmq_ctx_new();
    zmqx.connection = zmq_socket(zmqx.context, ZMQ_REQ);
    char* addr = malloc(30);
    sprintf(addr, "tcp://localhost:%d", port);
    int rc = zmq_connect(zmqx.connection, addr);
    return zmqx;
}

void zmqx_close(zmqx zmqx) {
    zmq_close(zmqx.connection);
    zmq_ctx_destroy(zmqx.context);
}

void zmqx_send(zmqx zmqx, char* msg) {
    int x = strlen(msg);
    zmq_send(zmqx.connection, msg, strlen(msg), 0);
}

char* zmqx_recv(zmqx zmqx) {
    int const BUF_SIZE = 1024;
    char* buf = malloc(BUF_SIZE);
    int len = zmq_recv(zmqx.connection, buf, BUF_SIZE, 0);
    buf[len] = 0;
    return buf;
}