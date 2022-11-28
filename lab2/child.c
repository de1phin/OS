#include "stdio.h"
#include "unistd.h"
#include "sys/wait.h"
#include "string.h"
#include "stdlib.h"

#define BUFFER_SIZE 1024

void to_upper(char* string, int len) {
    for (int i = 0; i < len; i++) {
        if (string[i] >= 'a' && string[i] <= 'z') {
            string[i] += 'A' - 'a';
        }
    }
}

void child1() {
    char* buffer = malloc(sizeof(char)*BUFFER_SIZE);

    int len, err;
    while (1) {
        len = read(STDIN_FILENO, buffer, BUFFER_SIZE);

        if (buffer[0] == 0) {
            break;
        }

        to_upper(buffer, len);
        
        err = write(STDOUT_FILENO, buffer, len*sizeof(char));
        if (err == -1) {
            char* msg = "child1 error: failed to write\n";
            write(STDERR_FILENO, msg, strlen(msg));
            break;
        }
    }

    buffer[0] = 0;
    write(STDOUT_FILENO, buffer, 1);
    free(buffer);
}

int delete_extra_spaces(char* string, int len) {
    int resulting_len = 1;
    for (int i = 1; i < len; i++) {
        string[resulting_len] = string[i];
        if (!(string[i - 1] == ' ' && string[i] == ' ')) {
            resulting_len++;
        }
    }
    return resulting_len;
}

void child2() {
    char* buffer = malloc(sizeof(char)*BUFFER_SIZE);

    int len, err;
    while (1) {
        len = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (len <= 0) {
            char* msg = "child2 error: failed to read\n";
            write(STDERR_FILENO, msg, strlen(msg));
            break;
        }

        if (buffer[0] == 0) {
            break;
        }

        len = delete_extra_spaces(buffer, len);
        
        err = write(STDOUT_FILENO, buffer, len*sizeof(char));
        if (err == -1) {
            char* msg = "child2 error: failed to write\n";
            write(STDERR_FILENO, msg, strlen(msg));
            break;
        }
    }

    free(buffer);
}

int main(int argc, char** argv) {

    int fd[2];
    int err;

    err = pipe(fd);
    if (err == -1) {
        char* msg = "child error: failed to create pipe\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd[1], STDOUT_FILENO);
        child1();
    } else {
        dup2(fd[0], STDIN_FILENO);
        child2();
    }

    close(fd[0]);
    close(fd[1]);

    wait(NULL);

    return 0;
}