#include "stdio.h"
#include "unistd.h"
#include "sys/wait.h"
#include "stdlib.h"
#include "signal.h"
#include "string.h"
#include "sys/mman.h"

#define BUFSIZE 1024

int file_fd; char* filename;
char* data;

int run_child() {
    pid_t pid = fork();
    if (pid < 0) {
        return 0;
    }
    if (pid != 0) {
        return pid;
    }

    dup2(file_fd, STDIN_FILENO);
    
    int err = execl("./child", "./child", NULL);
    if (err == -1) {
        printf("failed to exec ./child\n");
        return 0;
    }
}

pid_t child_pid = 0;

void quit() {
    wait(NULL);

    close(file_fd);
    remove(filename);
    free(filename);
    char* msg = "parent quits\n";
    write(STDERR_FILENO, msg, strlen(msg));
    exit(0);
}

void handle_error(char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    kill(child_pid, SIGINT);
    quit();
}

void debug(char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
}

void start() {
    size_t n = BUFSIZE;
    int len = getline(&data, &n, stdin);
    if (len <= 0) {
        perror("getline\n");
        quit();
    }

    kill(child_pid, SIGUSR1);
}

void finish(int sig) {
    if (sig != SIGUSR2) {
        handle_error("unexpected call to finish\n");
    }

    int err = write(STDOUT_FILENO, data, strlen(data));
    if (err == -1) {
        handle_error("failed to write to stdout\n");
    }

    start();
}

void sig_handler(int sig) {
    char* msg = "parent gets signal\n";
    write(STDERR_FILENO, msg, strlen(msg));
    quit();
}

int main(int argc, char** argv) {
    signal(SIGINT, sig_handler);

    filename = malloc(sizeof(char)*32);
    strcpy(filename, "/var/tmp/lab4-XXXXXX");
    file_fd = mkstemp(filename);
    if (file_fd < 0) {
        perror("mkstemp");
        return 1;
    }

    ftruncate(file_fd, BUFSIZE);

    data = mmap(NULL, BUFSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap\n");
        return 1;
    }

    child_pid = run_child();

    if (child_pid == 0) {
        printf("failed to run child\n");
        return 1;
    }

    signal(SIGUSR2, finish);
    start();
    while (1) { sleep(1); };

    return 0;
}