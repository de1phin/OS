#define _COMMON_C
#include "common.h"

int id;
int parent_id;
int child_id;
bool is_master;
void* responder;
void* requester = NULL;
char* reqbuf;
char* msgbuf;
char* addr;
unsigned int heartbit_ms;

int broker_port;

void* context = NULL;

int connect_to_vacant_port(void* responder, int min_port, int max_port) {
    int rc = 0;
    for (int port = min_port; port <= max_port; port++) {
        sprintf(addr, "tcp://*:%d", port);
        rc = zmq_bind(responder, addr);
        if (rc == 0) {
            free(addr);
            return port;
        }
        if (errno != EADDRINUSE) {
            free(addr);
            return -1;
        }
    }
    free(addr);
    errno = EADDRINUSE;
    return -1;
}


pthread_mutex_t mq_mu;

void init_mq(int _broker_port) {
    pthread_mutex_init(&mq_mu, NULL);
    addr = malloc(16);
    broker_port = _broker_port;
    context = zmq_ctx_new();

    responder = zmq_socket(context, ZMQ_REP);
    int port = connect_to_vacant_port(responder, 10000, 60000);

    requester = zmq_socket(context, ZMQ_REQ);
    char* buf = malloc(32);
    sprintf(buf, "tcp://localhost:%d", broker_port);
    zmq_connect(requester, buf);

    sprintf(buf, "mq:init:port=%d:id=%d", port, id);
    zmq_send(requester, buf, strlen(buf), 0);
    zmq_recv(requester, buf, 1, 0); // ack
    free(buf);
}

void stop_mq() {
    char* buf = malloc(32);
    sprintf(buf, "mq:disconnect:id=%d", id);
    zmq_send(requester, buf, strlen(buf), 0);
    zmq_recv(requester, buf, 1, 0);
    free(buf);
    zmq_unbind(responder, addr);
    free(addr);
    zmq_close(requester);
    zmq_ctx_destroy(context);
}

char* buffer;
int buffer_len;
char* response;
int response_len;

void send_via_mq(int id, char* message) {
    int len = strlen(message);
    char* buffer = malloc(len+40);
    len = sprintf(buffer, "mq:send:id=%d:message=%s", id, message);
    pthread_mutex_lock(&mq_mu);
    zmq_send(requester, buffer, len, 0);
    zmq_recv(requester, buffer, 1, 0); // ack
    pthread_mutex_unlock(&mq_mu);
    free(buffer);
}

void create_child(int _id) {
    if (fork() != 0)
        return;
    char* id_str = malloc(10);
    sprintf(id_str, "%d", _id);
    char* parent_id_str = malloc(10);
    sprintf(parent_id_str, "%d", id);
    execl("./main", "./main", id_str, parent_id_str, NULL);
}