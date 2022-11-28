#include "stdio.h"
#include "unistd.h"
#include "sys/wait.h"
#include "stdlib.h"

#define BUFFER_SIZE 1024

int run_child(int fd_in, int fd_out) {
    pid_t pid = fork();
    if (pid < 0) {
        return 0;
    }
    if (pid != 0) {
        return pid;
    }

    dup2(fd_in, STDIN_FILENO);
    dup2(fd_out, STDOUT_FILENO);
    
    int err = execl("./child", "./child", NULL);
    if (err == -1) {
        printf("failed to exec ./child\n");
        return 0;
    }
}

void mainloop(int fd_in, int fd_out) {
    size_t bufSize = BUFFER_SIZE*sizeof(char);
    char* buffer = malloc(bufSize);
    int len, err;

    while (1) {
        len = getline(&buffer, &bufSize, stdin);
        if (len <= 0) {
            break;
        }
        
        if (buffer[0] == '\n') {
            break;
        }
        
        err = write(fd_out, buffer, len*sizeof(char));
        if (err == -1) {
            printf("failed to write\n");
            break;
        }
        
        len = read(fd_in, buffer, sizeof(char)*BUFFER_SIZE);
        if (len <= 0) {
            break;
        }
        buffer[len] = 0;
        printf("%s", buffer);
    }
    
    buffer[0] = 0;
    write(fd_out, buffer, 1);
    free(buffer);
}

int main(int argc, char** argv) {
    int err;

    int fd1[2], fd2[2];
    err = pipe(fd1);
    if (err == -1) {
        printf("failed to create pipe to child1\n");
        return 1;
    }
    err = pipe(fd2);
    if (err == -1) {
        printf("failed to create pipe to child2\n");
        return 1;
    }

    pid_t child_pid = run_child(fd1[0], fd2[1]);

    if (child_pid == 0) {
        printf("failed to run child\n");
        return 1;
    }

    mainloop(fd2[0], fd1[1]);

    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);
    wait(NULL);

    return 0;
}