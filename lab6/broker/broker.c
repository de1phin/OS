#include "broker.h"
#include <pthread.h>

typedef struct {
    pthread_mutex_t mu;
    void* connection;
    int id;
} t_pub;

typedef struct pub_list {
    t_pub pub;
    struct pub_list* next;
} t_pub_list;

t_pub_list list;

void* context;
char* response;
int resp_len;

bool handle_mq_init(char* raw_message) {
    int port, id;
    if (sscanf(raw_message, "mq:init:port=%d:id=%d", &port, &id) != 2) {
        return false;
    }
    
    t_pub_list* newpub = NULL;
    for (t_pub_list* ptr = list.next; ptr != NULL; ptr = ptr->next) {
        if (ptr->pub.id == id) {
            newpub = ptr;
            if (ptr->pub.connection) {
                zmq_close(ptr->pub.connection);
                ptr->pub.connection = NULL;
            }
            break;
        }
    }

    if (newpub == NULL) {
        newpub = malloc(sizeof(t_pub_list));
        newpub->pub.id = id;
        newpub->next = list.next;
        pthread_mutex_init(&newpub->pub.mu, NULL);
        list.next = newpub;
    }
    newpub->pub.connection = zmq_socket(context, ZMQ_REQ);
    char* addr = malloc(30);
    sprintf(addr, "tcp://localhost:%d", port);
    assert(zmq_connect(newpub->pub.connection, addr) == 0);
    free(addr);
    return true;
}

bool handle_mq_send(char* raw_message) {
    char* message = malloc(BUFFER_SIZE);
    int id;
    if (sscanf(raw_message, "mq:send:id=%d:message=%900[0-9a-zA-Z:=_ -]", &id, message) != 2) {
        free(message);
        return false;
    }

    for (t_pub_list* ptr = list.next; ptr != NULL; ptr = ptr->next) {
        if (ptr->pub.id != id) {
            continue;
        }

        pthread_mutex_lock(&ptr->pub.mu);
        zmq_send(ptr->pub.connection, message, strlen(message), 0);
        zmq_recv(ptr->pub.connection, message, 1, 0); //ack
        pthread_mutex_unlock(&ptr->pub.mu);
        return true;
    }

    free(message);
    return true;
}

bool handle_mq_disconnect(char* raw_message) {
    int id;
    if (sscanf(raw_message, "mq:disconnect:id=%d", &id) != 1) {
        return false;
    }

    for (t_pub_list* ptr = &list; ptr->next != NULL; ptr = ptr->next) {
        if (ptr->next->pub.id != id)
            continue;
        t_pub_list* node = ptr->next;
        ptr->next = node->next;
        free(node);
        return true;
    }
    return false;
}

void handle(void* msg) {
    if (handle_mq_init((char*)msg));
    else if (handle_mq_send((char*)msg));
    else if (handle_mq_disconnect((char*)msg));
    free(msg);
}

void start(int port) {
    context = zmq_ctx_new();
    void* responder = zmq_socket(context, ZMQ_REP);
    char* addr = malloc(20);
    sprintf(addr, "tcp://*:%d", port);
    int rc = zmq_bind(responder, addr);
    printf("%s\n", addr);
    free(addr);
    assert(rc == 0);

    list.next = NULL;
    response = malloc(BUFFER_SIZE);

    char* buffer = malloc(BUFFER_SIZE);

    while (1) {
        int len = zmq_recv(responder, buffer, BUFFER_SIZE, 0);
        buffer[len] = 0;
        zmq_send(responder, "\0", 1, 0); // ack
        char* handle_buf = malloc(len+1);
        memcpy(handle_buf, buffer, len+1);
        write(STDOUT_FILENO, buffer, len);
        write(STDOUT_FILENO, "\n", 1);
        pthread_t thread;
        pthread_create(&thread, NULL, (void*)handle, (void*)handle_buf);
    }
    free(buffer);
}
