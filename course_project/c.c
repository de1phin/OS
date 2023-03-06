#include "common/zmqx.h"
#include "common/ports.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "string.h"

zmqx A, B;

void sighandler() {
    zmqx_close(A);
    zmqx_close(B);
    exit(0);
}

int main() {
    signal(SIGINT, sighandler);

    A = zmqx_new_listener(A_TO_C_PORT);
    B = zmqx_new_writer(C_TO_B_PORT);

    while (1) {
        char* msg = zmqx_recv(A);
        printf("%s\n", msg);
        zmqx_send(A, "\0");
        sprintf(msg, "%ld", strlen(msg));
        zmqx_send(B, msg);
        char* ack = zmqx_recv(B);
        free(msg);
        free(ack);
    }
    return 0;
}
