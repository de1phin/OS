#include "stdio.h"
#include "unistd.h"
#include "sys/wait.h"
#include "sys/mman.h"
#include "string.h"
#include "signal.h"
#include "stdlib.h"

#define BUFSIZE 1024

void debug(char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

void to_upper(char* string, int len) {
    for (int i = 0; i < len; i++) {
        if (string[i] >= 'a' && string[i] <= 'z') {
            string[i] += 'A' - 'a';
        }
    }
}

char* data;
pid_t child_pid = 0;

void quit() {
    wait(NULL);
    
    debug("child quits\n");
    exit(0);
}

void handle_error(char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    if (child_pid)
        kill(child_pid, SIGINT);
    quit();
}

void child1(int sig) {
    if (sig != SIGUSR1) {
        handle_error("unexpected call to child1\n");
    }

    to_upper(data, strlen(data));

    kill(child_pid, SIGUSR1);
}

void delete_extra_spaces(char* string, int len) {
    int resulting_len = 1;
    for (int i = 1; i < len; i++) {
        string[resulting_len] = string[i];
        if (!(string[i - 1] == ' ' && string[i] == ' ')) {
            resulting_len++;
        }
    }
    string[resulting_len] = 0;
}

void sig_to_parent(int sig) {
    if (sig != SIGUSR2) {
        handle_error("unexpected call to sig_to_parent\n");
    }

    kill(getppid(), SIGUSR2);
}

void child2(int sig) {
    if (sig != SIGUSR1) {
        handle_error("unexpected call to child2\n");
    }

    delete_extra_spaces(data, strlen(data));
    
    sig_to_parent(SIGUSR2);
}

void sig_handler(int sig) {
    debug("child gets signal\n");
    quit();
}

int main(int argc, char** argv) {
    signal(SIGINT, sig_handler);
    signal(SIGUSR2, sig_to_parent);

    child_pid = fork();    
    
    data = mmap(NULL, BUFSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, STDIN_FILENO, 0);
    if (data == MAP_FAILED) {
        perror("mmap\n");
        return 1;
    }

    if (child_pid == 0) {
        signal(SIGUSR1, child2);
    } else {
        signal(SIGUSR1, child1);
    }
    while(1) { sleep(1); };

    return 0;
}