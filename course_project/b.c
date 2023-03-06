#include "common/zmqx.h"
#include "common/ports.h"
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "signal.h"

zmqx A, C;

int quit = 0;

void listen_a() {
    while (1) {
        char* msg = zmqx_recv(A);
        if (quit) return;
        zmqx_send(A, "\0");
        printf("From A: %s\n", msg);
        free(msg);
    }
}

void listen_b() {
    while (1) {
        char* msg = zmqx_recv(C);
        if (quit) return;
        zmqx_send(C, "\0");
        printf("From C: %s\n", msg);
        free(msg);
    }
}

void sighandler() {
    quit = 1;
    zmqx_close(A);
    zmqx_close(C);
    exit(0);
}

int main() {
    signal(SIGINT, sighandler);

    A = zmqx_new_listener(A_TO_B_PORT);
    C = zmqx_new_listener(C_TO_B_PORT);

    pthread_t thread;
    pthread_create(&thread, NULL, (void*)listen_a, NULL);
    listen_b();
    pthread_join(thread, NULL);
    return 0;
}