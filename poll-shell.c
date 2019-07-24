#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PARENT  0
#define CHILD   1

int handle_parent(int sockfd);
int handle_child(char *cmd);

int socks[2];

int main(void)
{
    pid_t pid;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socks) == -1) {
        perror("socketpair");
        exit(1);
    }

    pid = fork();
    if (-1 == pid) {
        // Failure
        perror("fork");
        exit(1);
    }

    if (0 == pid) {
        // Child process
        close(socks[PARENT]);
        handle_child("sh");
    } else {
        // Parent process
        close(socks[CHILD]);
        handle_parent(socks[PARENT]);
    }

    return 0;
}

int handle_parent(int sockfd)
{
    char buf[4096];
    ssize_t n_read;

    do {
        n_read = read(sockfd, buf, sizeof(buf));
        puts(buf);
        memset(buf, 0, n_read);
    } while (n_read > 0);

    if (-1 == n_read) {
        perror("read");
        exit(1);
    }

    wait(NULL);

    return 0;
}

int handle_child(char *cmd)
{
    dup2(socks[CHILD], STDOUT_FILENO);
    char* const args[] = {"/bin/sh", "-c", cmd, NULL };
    execv("/bin/sh", args);

    return 0;
}

