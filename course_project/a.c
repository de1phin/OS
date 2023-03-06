#include "common/zmqx.h"
#include "common/ports.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "string.h"

zmqx B, C;

void sighandler() {
    zmqx_close(B);
    zmqx_close(C);
    exit(0);
}

int main() {
    B = zmqx_new_writer(A_TO_B_PORT);
    C = zmqx_new_writer(A_TO_C_PORT);

    signal(SIGINT, sighandler);

    int const BUF_SIZE = 1024;
    char* buffer = malloc(BUF_SIZE);

    while (1) {
        scanf("%s", buffer);
        zmqx_send(C, buffer);
        char* msg = zmqx_recv(C);
        sprintf(buffer, "%ld", strlen(msg));
        zmqx_send(B, buffer);
        char* ack = zmqx_recv(B);
        free(msg);
        free(ack);
    }
    return 0;
}
