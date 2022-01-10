#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#define _XOPEN_SOURCE
#include <unistd.h>

#include "rdevkit.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

int main()
{
	int sockfd;
	char *slave_path;
	struct sockaddr_in servaddr, cli;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		exit(0);
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(PORT);

	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0)
		exit(0);

	struct dk_message msg;
	for (;;)
	{
		receive_dk_msg(sockfd, &msg);
		printf("OPCODE: %d DATA: %d\n", msg.opcode, msg.data);
		switch(msg.opcode)
		{
			case R_MSG_INPUT:
				printf("Input caught!\n");
				printf("type %d, code %d, state %d\n");
				break;
			case R_MSG_SPIERDALAJ:
				printf("spierdalaj\n");
				quit_gracefully(sockfd);
				break;
		}
	}

	send_dk_msg(sockfd, R_MSG_SPIERDALAJ, 0, 0, 0);
	close(sockfd);
}
