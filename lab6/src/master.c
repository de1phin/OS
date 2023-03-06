#include "master.h"

t_node* node_list;
bool heartbit_is_set;

void prompt() {
    write(STDOUT_FILENO, "master:~$ ", 10);
}

long long get_ts() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return ((long long)t.tv_sec)*1000 + ((long long)t.tv_usec)/1000;
}

void master_send(int id, char* msg) {
    if (id == -1) {
        for (t_node* node = node_list; node; node=node->next)
            if (node->is_list_root)
                send_via_mq(node->id, msg);
        return;
    }

    for (t_node* node = node_list; node; node=node->next) {
        if (node->id == id) {
            send_via_mq(node->list_root_id, msg);
            return;
        }
    }
    
    write(STDOUT_FILENO, "Error: Node does not exist\n", 27);
}

void master_create_child(int id) {
    t_node* node = malloc(sizeof(t_node));
    node->alive = false;
    node->id=id;
    node->list_root_id=id;
    node->is_list_root = true;
    node->last_heartbit_ts = get_ts();
    node->next=node_list;
    node_list = node;
    create_child(id);
}

bool master_handle_create_exists(char* msg) {
    int node_id, node_parent_id;
    if (sscanf(msg, "create:exists:id=%d:parent_id=%d", &node_id, &node_parent_id) != 2)
        return false;
    
    char* text = malloc(64);
    sprintf(text, "\nError: node with id %d already exists\nmaster:~$ ", node_id);
    write(STDOUT_FILENO, text, strlen(text));
    free(text);
    return true;
}

bool master_handle_create_parent_busy(char* msg) {
    int parent_id;
    if (sscanf(msg, "create:parent busy:parent_id=%d", &parent_id) != 1)
        return false;
    
    char* text = malloc(64);
    sprintf(text, "\nCreate: Error: node %d already has a child node\n", parent_id);
    write(STDOUT_FILENO, text, strlen(text));
    free(text);
    prompt();
    return true;
}

bool master_handle_create_ok(char* msg) {
    int node_id, node_parent_id;
    if (sscanf(msg, "create:ok:id=%d:parent_id=%d", &node_id, &node_parent_id) != 2)
        return false;
    
    if (node_parent_id == id) {
        master_create_child(node_id);
        return true;
    }

    char* text = malloc(64);
    sprintf(text, "\nError: node with id %d does not exist\nmaster:~$ ", node_parent_id);
    write(STDOUT_FILENO, text, strlen(text));
    free(text);
    return true;
}

bool master_handle_remove(char* msg) {
    int node_id;
    if (sscanf(msg, "remove:ok:id=%d", &node_id) == 1) {
        // remove node from lists
        if (node_list && node_list->id == node_id) {
            t_node* ptr = node_list;
            node_list = node_list->next;
            free(ptr);
        } else {
            for (t_node* ptr = node_list; ptr->next; ptr = ptr->next) {
                if (ptr->next->id != node_id)
                    continue;
                t_node* tmp = ptr->next;
                ptr->next = tmp->next;
                free(tmp);
                break;
            }
        }
        write(STDOUT_FILENO, "Remove: Ok\n", 11);
        prompt();
        return true;
    }

    if (sscanf(msg, "remove:not found:id=%d", &node_id)) {
        write(STDOUT_FILENO, "Remove: Error: Not found\n", 25);
        prompt();
        return true;
    }

    return false;
}

bool master_handle_ready(char* msg) {
    int node_id, node_parent_id, pid;
    if (sscanf(msg, "ready:id=%d:parent_id=%d:pid=%d", &node_id, &node_parent_id, &pid) != 3)
        return false;
    
    if (node_parent_id != id) {
        // add node to node list
        t_node* node = malloc(sizeof(node));
        node->id = node_id;
        // assume it is alive at this moment
        node->last_heartbit_ts = get_ts();
        node->alive = true;
        for (t_node* _node = node_list; _node; _node = _node->next) {
            if (_node->id == node_parent_id) {
                node->list_root_id = _node->list_root_id;
                break;
            }
        }
        node->next = node_list;
        node->is_list_root = false;
        node_list = node;
    }

    if (heartbit_is_set) {
        // set heartbit for newly created node
        sprintf(msgbuf, "heartbit:set:%d", heartbit_ms);
        master_send(node_id, msgbuf);
    }

    sprintf(msgbuf, "\nOk: created node with id = %d and pid = %d\n", node_id, pid);
    write(STDOUT_FILENO, msgbuf, strlen(msgbuf));
    prompt();

    return true;
}

bool master_handle_exec_ans(char* msg) {
    int sum;
    if (sscanf(msg, "exec:sum=%d", &sum) != 1)
        return false;
    
    sprintf(msgbuf, "\nOk: sum is %d\n", sum);
    write(STDOUT_FILENO, msgbuf, strlen(msgbuf));
    prompt();
    return true;
}

bool master_handle_exec_errnotexist(char* msg) {
    int _id;
    if (sscanf(msg, "exec:errnotexist:id=%d", &_id) != 1)
        return false;
    
    sprintf(msgbuf, "\nError: node with id %d does not exist\n", _id);
    write(STDOUT_FILENO, msgbuf, strlen(msgbuf));
    prompt();
    return true;
}

bool master_handle_heartbit(char* msg) {
    int _id;
    if (sscanf(msg, "heartbit:id=%d", &_id) != 1)
        return false;
    
    for (t_node* ptr = node_list; ptr != NULL; ptr = ptr->next) {
        if (ptr->id != _id)
            continue;
        ptr->last_heartbit_ts = get_ts();
        break;
    }
    
    return true;
}

bool master_handle_child(char* msg) {
    int _id;
    if (sscanf(msg, "child:id=%d", &_id) != 1)
        return false;
    
    int list_root_id = -1;
    for (t_node* node = node_list; node; node = node->next) {
        if (node->id == _id) {
            list_root_id = node->list_root_id;
            break;
        }
    }
    assert(list_root_id >= 0);

    for (t_node* node = node_list; node; node = node->next) {
        if (node->list_root_id == list_root_id) {
            node->list_root_id = _id;
        }
    }
}

void listen_mq(void) {
    while (1) {
        int len = zmq_recv(responder, reqbuf, BUFFER_SIZE, 0);
        reqbuf[len] = 0;
        zmq_send(responder, "\0", 1, 0); // ack
        if (master_handle_exec_errnotexist(reqbuf));
        else if (master_handle_create_exists(reqbuf));
        else if (master_handle_create_ok(reqbuf));
        else if (master_handle_exec_ans(reqbuf));
        else if (master_handle_remove(reqbuf));
        else if (master_handle_ready(reqbuf));
        else if (master_handle_heartbit(reqbuf));
        else if (master_handle_create_parent_busy(reqbuf));
        else (master_handle_child(reqbuf));
    }
}

void heartbit_check_routine() {
    // reset heartbit
    long long ts = get_ts();
    for (t_node* node = node_list; node; node=node->next)
        node->last_heartbit_ts = ts;

    const int check_interval = 100000;
    char* buf = malloc(64);
    while (1) {
        usleep(check_interval);
        long long heartbit_timeout = (long long)heartbit_ms*4;
        ts = get_ts();
        bool written_stuff = false;
        for (t_node* ptr = node_list; ptr != NULL; ptr = ptr->next) {
            if (ts - ptr->last_heartbit_ts <= heartbit_timeout) {
                ptr->alive = true;
                continue;
            }
            if (!ptr->alive) continue;
            ptr->alive = false;
            if (!written_stuff) 
                write(STDOUT_FILENO, "\n", 1);
            written_stuff = true;
            sprintf(buf, "Heartbit: node %d is unavailable now\n", ptr->id);
            write(STDOUT_FILENO, buf, strlen(buf));
        }
        if (written_stuff)
            prompt();
    }
    free(buf);
}

bool create_command(char* msg) {
    int _id;
    int parent_id;
    if (sscanf(msg, "create %d %d", &_id, &parent_id) != 2)
        return false;
    
    for (t_node* node = node_list; node; node=node->next) {
        if (node->id == _id) {
            write(STDOUT_FILENO, "Error: id already exists\n", 25);
            return true;
        }
    }

    if (_id == parent_id) {
        write(STDOUT_FILENO, "Error: id must not be equal to parent_id\n", 42);
        prompt();
        return true;
    }

    if (parent_id == id) {
        master_create_child(_id);
        return true;
    }

    char* create_msg = malloc(30);
    sprintf(create_msg, "create:check:id=%d:parent_id=%d", _id, parent_id);
    master_send(parent_id, create_msg);
    free(create_msg);

    return true;
}

bool exec_command(char* msg) {
    int _id;
    int n;
    char* values = malloc(512);
    if (sscanf(msg, "exec %d %d %256[0-9 ]", &_id, &n, values) != 3) {
        free(values);
        return false;
    }

    sprintf(msgbuf, "exec:id=%d:n=%d:values=%s", _id, n, values);
    master_send(_id, msgbuf);
    free(values);
    return true;
}

bool remove_command(char* msg) {
    int node_id;
    if (sscanf(msg, "remove %d", &node_id) != 1) {
        return false;
    }

    if (node_id <= 0) {
        write(STDOUT_FILENO, "EINVAL\n", 7);
        return true;
    }

    sprintf(msgbuf, "remove:id=%d", node_id);
    master_send(node_id, msgbuf);
    return true;
}

bool heartbit_command(char* msg) {
    int ms;
    if (sscanf(msg, "heartbit %d", &ms) != 1)
        return false;
    
    heartbit_ms = ms;
    if (!heartbit_is_set) {
        pthread_t thread;
        pthread_create(&thread, NULL, (void*)heartbit_check_routine, NULL);
    }
    heartbit_is_set = true;
    sprintf(msgbuf, "heartbit:set:%d", ms);
    master_send(-1, msgbuf);
    return true;
}

void read_stdin() {
    char* stdin_buf = malloc(BUFFER_SIZE);
    while (1) {
        prompt();
        int len = read(STDIN_FILENO, stdin_buf, BUFFER_SIZE);
        stdin_buf[len] = 0;
        if (create_command(stdin_buf));
        else if (exec_command(stdin_buf));
        else if (remove_command(stdin_buf));
        else if (heartbit_command(stdin_buf));
    }
    free(stdin_buf);
}

void run_master(int broker_port) {
    heartbit_is_set = false;
    reqbuf = malloc(BUFFER_SIZE);
    msgbuf = malloc(BUFFER_SIZE);
    is_master == true;
    id = 0;
    parent_id = -1;
    child_id = -1;
    init_mq(broker_port);

    pthread_t thread;
    pthread_create(&thread, NULL, (void*)listen_mq, NULL);

    read_stdin();
}
