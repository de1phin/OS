#include "node.h"
#include "common.h"

bool stopped;
char* heartbit_msg;

void quit() {
    stopped = true;
    stop_mq();
    free(msgbuf);
    free(reqbuf);
    free(heartbit_msg);
    exit(0);
}

bool handle_create_check(char* msg) {
    int node_id = 0, node_parent_id = 0;
    if (sscanf(msg, "create:check:id=%d:parent_id=%d", &node_id, &node_parent_id) != 2)
        return false;

    if (node_id == id) {
        sprintf(msgbuf, "create:exists:id=%d:parent_id=%d", node_id, node_parent_id);
        send_via_mq(parent_id, msgbuf);
        return true;
    }

    if (node_parent_id == id && child_id != -1) {
        sprintf(msgbuf, "create:parent busy:parent_id=%d", id);
        send_via_mq(parent_id, msgbuf);
        return true;
    }

    if (child_id == -1) {
        if (node_parent_id == id) {
            create_child(node_id);
            return true;
        }
        sprintf(msgbuf, "create:ok:id=%d:parent_id=%d", node_id, node_parent_id);
        send_via_mq(parent_id, msgbuf);
        return true;
    }

    send_via_mq(child_id, msg);
    return true;
}

bool handle_create_parent_busy(char* msg) {
    int _parent_id;
    if (sscanf(msg, "create:parent busy:parent_id=%d", &_parent_id) != 1)
        return false;
    
    send_via_mq(parent_id, msg);
    return true;
}

bool handle_create_exists(char* msg) {
    int node_id = 0, node_parent_id = 0;
    if (sscanf(msg, "create:exists:id=%d:parent_id=%d", &node_id, &node_parent_id) != 2)
        return false;

    send_via_mq(parent_id, msg);
    return true;
}

bool handle_create_ok(char* msg) {
    int node_id = 0, node_parent_id = 0;
    if (sscanf(msg, "create:ok:id=%d:parent_id=%d", &node_id, &node_parent_id) != 2)
        return false;

    if (node_parent_id == id) {
        create_child(node_id);
        return true;
    }

    send_via_mq(parent_id, msg);
    return true;
}

bool handle_parent(char* msg) {
    int new_parent_id;
    if (sscanf(msg, "parent:id=%d", &new_parent_id) != 1) 
        return false;
    parent_id = new_parent_id;
    return true;
}

bool handle_ready(char* msg) {
    int node_id, node_parent_id, pid;
    if (sscanf(msg, "ready:id=%d:parent_id=%d:pid=%d", &node_id, &node_parent_id, &pid) != 3)
        return false;
    
    send_via_mq(parent_id, msg);
    if (node_parent_id != id)
        return true;
    
    if (child_id != -1) {
        sprintf(msgbuf, "parent:id=%d", node_id);
        send_via_mq(child_id, msgbuf);
        sprintf(msgbuf, "child:id=%d", child_id);
        send_via_mq(node_id, msgbuf);
    }
    child_id = node_id;
    return true;
}

bool handle_child(char* msg) {
    int _id;
    if (sscanf(msg, "child:id=%d", &_id) != 1)
        return false;
    child_id = _id;
    return true;
}

bool handle_remove(char* msg) {
    int node_id;
    if (sscanf(msg, "remove:id=%d", &node_id) != 1)
        return false;
    
    if (node_id == id) {
        if (child_id != -1) {
            sprintf(msgbuf, "parent:id=%d", parent_id);
            send_via_mq(child_id, msgbuf);
            sprintf(msgbuf, "child:id=%d", child_id);
            send_via_mq(parent_id, msgbuf);
        }
        sprintf(msgbuf, "remove:ok:id=%d", id);
        send_via_mq(parent_id, msgbuf);
        quit();
        return true;
    }

    if (child_id == -1) {
        sprintf(msgbuf, "remove:not found:id=%d", node_id);
        send_via_mq(parent_id, msgbuf);
        return true;
    }

    send_via_mq(child_id, msg);
    return true;
}

bool handle_remove_not_found(char *msg) {
    int node_id;
    if (sscanf(msg, "remove:not found:id=%d", &node_id) != 1)
        return false;
    
    send_via_mq(parent_id, msg);
    return true;
}

bool handle_remove_ok(char* msg) {
    int node_id;
    if (sscanf(msg, "remove:ok:id=%d", &node_id) != 1)
        return false;
    
    if (node_id == child_id)
        child_id = -1;
    
    send_via_mq(parent_id, msg);
    return true;
}

int calc_sum(int n, char* values) {
    int len = strlen(values);
    int sum = 0;
    int p = 0;
    int integer = 0;
    for (int i = 0; i < n; i++) {
        for (; p < len && values[p] != ' '; p++) {
            integer *= 10;
            integer += values[p] - '0';
        }
        sum += integer;
        integer = 0;
        p++; // ' '
    }
    return sum;
}

bool handle_exec(char* msg) {
    int _id, n;
    char* values = malloc(512);
    if (sscanf(msg, "exec:id=%d:n=%d:values=%256[0-9 ]", &_id, &n, values) != 3) {
        free(values);
        return false;
    }

    if (_id == id) {
        int sum = calc_sum(n, values);
        free(values);
        sprintf(msgbuf, "exec:sum=%d", sum);
        send_via_mq(parent_id, msgbuf);
        return true;
    }
    free(values);

    if (child_id == -1) {
        sprintf(msgbuf, "exec:errnotexist:id=%d", _id);
        send_via_mq(parent_id, msgbuf);
        return true;
    }
    
    send_via_mq(child_id, msg);
    return true;
}

bool handle_exec_sum(char* msg) {
    int sum;
    if (sscanf(msg, "exec:sum=%d", &sum) != 1)
        return false;
    
    send_via_mq(parent_id, msg);
    return true;
}

bool handle_exec_errnotexist(char* msg) {
    int _id;
    if (sscanf(msg, "exec:errnotexist:id=%d", &_id) != 1)
        return false;
    
    send_via_mq(parent_id, msg);
    return true;
}

void heartbit() {
    send_via_mq(parent_id, heartbit_msg);
}

void run_heartbit() {
    while (1) {
        usleep(heartbit_ms);
        if (stopped) break;
        heartbit();
    }
}

bool handle_set_heartbit(char* msg) {
    int ms;
    if (sscanf(msg, "heartbit:set:%d", &ms) != 1)
        return false;
    
    heartbit_ms = ms*1000;

    if (child_id != -1)
        send_via_mq(child_id, msg);

    pthread_t thread;
    pthread_create(&thread, NULL, (void*)run_heartbit, NULL);

    return true;
}

bool handle_heartbit(char* msg) {
    int _id;
    if (sscanf(msg, "heartbit:id=%d", &_id) != 1)
        return false;
    send_via_mq(parent_id, msg);
    return true;
}

void listen_requests() {
    while (1) {
        int len = zmq_recv(responder, reqbuf, BUFFER_SIZE, 0);
        reqbuf[len] = 0;
        zmq_send(responder, "\0", 1, 0); // ack
        if (handle_create_parent_busy(reqbuf));
        else if (handle_exec_errnotexist(reqbuf));
        else if (handle_remove_not_found(reqbuf));
        else if (handle_create_exists(reqbuf));
        else if (handle_create_check(reqbuf));
        else if (handle_set_heartbit(reqbuf));
        else if (handle_create_ok(reqbuf));
        else if (handle_remove_ok(reqbuf));
        else if (handle_exec_sum(reqbuf));
        else if (handle_heartbit(reqbuf));
        else if (handle_parent(reqbuf));
        else if (handle_remove(reqbuf));
        else if (handle_ready(reqbuf));
        else if (handle_child(reqbuf));
        else if (handle_exec(reqbuf));
    }
}

void run_node(int broker_port, int _id, int _parent_id) {
    stopped = false;
    heartbit_msg = malloc(32);
    sprintf(heartbit_msg, "heartbit:id=%d", _id);

    is_master == false;
    id = _id;
    parent_id = _parent_id;
    child_id = -1;
    init_mq(broker_port);

    reqbuf = malloc(BUFFER_SIZE);
    msgbuf = malloc(BUFFER_SIZE);

    sprintf(reqbuf, "ready:id=%d:parent_id=%d:pid=%d", _id, _parent_id, getpid());
    send_via_mq(_parent_id, reqbuf);

    listen_requests();

    free(reqbuf);
    free(msgbuf);
}
