typedef struct {
    void* context;
    void* connection;
} zmqx;

zmqx zmqx_new_listener(int port);
zmqx zmqx_new_writer(int port);
void zmqx_close(zmqx);
void zmqx_send(zmqx, char*);
char* zmqx_recv(zmqx);
