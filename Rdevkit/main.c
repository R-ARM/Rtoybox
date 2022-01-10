#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <linux/input.h>

#include "rdevkit.h"

#define MAX 80
#define PORT 21370
#define SA struct sockaddr

struct dk_message message;

int connected;
int connfd = -1;
int sockfd;
int quit = 0;

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

pthread_t inHandler;
pthread_t accepter;

void set_color(int prio)
{
	switch(prio)
	{
		case R_M_LOG_ERR:
			printf("\033[0;31m");
			break;
		case R_M_LOG_WARN:
			printf("\033[0;33m");
			break;
		case R_M_LOG_INFO:
			printf("\033[0;32m");
			break;
		case R_M_LOG_DEBUG:
			printf("\033[0;36m");
			break;
		default:
			printf("\033[0;m");
			break;
	}
}

void handleInputMessages(void)
{
	for (;;)
	{
		if (connected == 1)
		{
			receive_dk_msg(connfd, &message);
			switch(message.opcode)
			{
				case R_MSG_LOG:
					set_color(message.data_alt);
					printf("%c", message.data);
					set_color(9999);
					break;
				case R_MSG_SPIERDALAJ:
					printf("quitting due to client\n");
					quit_gracefully(connfd);
					connected = 0;
					close(connfd);
					set_color(9999);
					break;
			}
		}
		else
			sleep(2);
	}
}

void handleConnections(void)
{
	int tmpconnfd, len;
	struct sockaddr_in cli;

	len = sizeof(cli);

	while (1)
	{
		tmpconnfd = accept(sockfd, &cli, &len);
		if (tmpconnfd < 0)
			continue;

		if (connected == 0)
		{
			connected = 1;
			connfd = tmpconnfd;
		}
		else
		{
			quit_gracefully(tmpconnfd);
			close(tmpconnfd);
		}
	}
}

int main()
{
	struct sockaddr_in servaddr, cli;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		exit(1);
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0)
		exit(2);

	if ((listen(sockfd, 5)) != 0)
		exit(3);

	pthread_create(&accepter, NULL, handleConnections, NULL);
	pthread_create(&inHandler, NULL, handleInputMessages, NULL);

	// SDL init and main loop
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		goto sdlerr;

	if (SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN, &window, &renderer) < 0)
		goto sdlerr;

	SDL_GL_SetSwapInterval(-1);
	
	if (TTF_Init() < 0)
		goto sdlerr;

	font = TTF_OpenFont("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", 20);
	if (font == NULL || window == NULL || renderer == NULL)
		goto sdlerr;

	SDL_Event event;
	int evState = 2;
	while (1)
	{
		SDL_RenderClear(renderer);
		
		while (SDL_PollEvent(&event) == 1)
		{
			evState = 2;
			switch (event.type)
			{
				case SDL_QUIT:
					quit_gracefully(connfd);
					connected = 0;
					close(connfd);
					close(sockfd);
					set_color(9999);
					exit(0);
					break;
				case SDL_KEYDOWN:
					evState = 1;
				case SDL_KEYUP:
					if (evState != 1)
						evState = 0;
					switch (event.key.keysym.sym)
					{
						case SDLK_UP:
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_DPAD_UP, evState);
							break;
						case SDLK_DOWN:

							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_DPAD_DOWN, evState);
							break;
						case SDLK_LEFT:
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_DPAD_LEFT, evState);
							break;
						case SDLK_RIGHT:
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_DPAD_RIGHT, evState);
							break;
						case SDLK_d: // A
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_EAST, evState);
							break;
						case SDLK_x: // B
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_SOUTH, evState);
							break;
						case SDLK_s: // X
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_NORTH, evState);
							break;
						case SDLK_z: // Y
							send_dk_msg(connfd, R_MSG_INPUT, EV_KEY, BTN_WEST, evState);
							break;
					}
					send_dk_msg(connfd, R_MSG_INPUT, EV_SYN, SYN_REPORT, 1);
			}
		}

		SDL_RenderPresent(renderer);
	}

out:
	quit_gracefully(connfd);
	close(connfd);
	close(sockfd);
	return 0;

sdlerr:
	printf("SDL2 Error: %s\n", SDL_GetError());
	close(connfd);
	close(sockfd);
	return 1;
}
