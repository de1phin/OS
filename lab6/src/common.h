#pragma once
#ifndef _COMMON_H
#define _COMMON_H

#include <zmq.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFFER_SIZE 256

#ifndef _COMMON_C
extern int id;
extern int parent_id;
extern int child_id;
extern bool is_master;
extern void* responder;
extern void* requester;
extern char* reqbuf;
extern char* msgbuf;
extern unsigned int heartbit_ms;
#endif

void init_mq(int);
void stop_mq();
void send_via_mq(int, char*);
void create_child(int);

bool handle_child(char*);

#endif
