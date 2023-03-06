#pragma once
#ifndef _MASTER_H
#define _MASTER_H

#include "stdio.h"
#include "common.h"
#include "sys/time.h"

typedef struct t_node_struct {
    int id;
    int list_root_id;
    long long last_heartbit_ts;
    struct t_node_struct* next;
    bool is_list_root;
    bool alive;
} t_node;

void run_master(int broker_port);
#endif
