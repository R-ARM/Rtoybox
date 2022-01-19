#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define _XOPEN_SOURCE
#include <unistd.h>

#include "Rdevkit/rdevkit.h"

#define ROS_PRIO_ERR	0
#define ROS_PRIO_WARN	1
#define ROS_PRIO_INFO	2
#define ROS_PRIO_DEBUG	3

#define ROS_INIT_SDL	1
#define ROS_INIT_TTF	2
#define ROS_INIT_INPUT	4

int _r_dk_socket_fd = -1;

int getBatLevel()
{
	if (getenv("ROS_BAT_LEVEL") == NULL)
		return 50;
	return atoi(getenv("ROS_BAT_LEVEL"));
}


#define log_err(f, ...)		r_log(ROS_PRIO_ERR, f, ##__VA_ARGS__)
#define log_warn(f, ...)	r_log(ROS_PRIO_WARN, f, ##__VA_ARGS__)
#define log_info(f, ...)	r_log(ROS_PRIO_INFO, f, ##__VA_ARGS__)
#define log_debug(f, ...)	r_log(ROS_PRIO_DEBUG, f, ##__VA_ARGS__)

const char* strings[] = {
	"Err:  ",
	"Warn: ",
	"Info: ",
	"Dbg:  ",
};

int r_log(int prio, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int n;
	char tmp[256];
	n = vsnprintf(tmp, 256, format, args);

	printf("%s", strings[prio]);
	int i = 0;
	while(i < n)
	{
		send_dk_msg(_r_dk_socket_fd, R_MSG_LOG, tmp[i], prio, 0);
		printf("%c", tmp[i]);
		i++;
	}
	
	va_end(args);
	return 0;
}

int (*_cb_input_handle)(int, int, int);
int _is_inp_hand_attach = 0;
int _r_reopen_joydev = 0;
pthread_t _r_js_thread;
pthread_t _r_dk_thread;

int r_attach_input_callback(int (*new_cb)(int, int, int))
{
	if (_is_inp_hand_attach == 0)
	{
		log_debug("Attaching new input handle\n");
		_cb_input_handle = new_cb;
		_is_inp_hand_attach = 1;
	}
	else
	{
		log_err("Attempted to attach input handle twice, bailing out\n");
		exit(1);
	}
}

void r_flush_input_events()
{
	_r_reopen_joydev = 1;
}

void _r_upd_joystick(void)
{
	int rd, joyfd, i;
	struct input_event ev[8];

	char dev[20];
	char name[32];

	while (1)
	{
		for (i = 0; i < 10; i++)
		{
			sprintf(dev, "/dev/input/event%d", i);
			joyfd = open(dev, O_RDONLY);
			if (ioctl(joyfd, EVIOCGNAME(32), name) < 0)
				close(joyfd);
			else
				if (!strcmp(name, "Rinputer"))
					goto found;
				else
					close(joyfd);
		}
		log_err("Missing Rinputer device!\n");
		while (sleep(10)); // do nothing now
found:

		while (_r_reopen_joydev == 0)
		{
			rd = read(joyfd, ev, sizeof(struct input_event) * 8);
			if (rd > 0)
			{
				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					if (_is_inp_hand_attach == 1)
					{
						_cb_input_handle(ev->type, ev->code, ev->value);
					}
					else
					{
						log_warn("Rinputer input event ignored!");
					}
				}
			}
		}
		close(joyfd);
		_r_reopen_joydev = 0;
	}
}

void _r_devkit_handler(void)
{
	struct sockaddr_in servaddr, cli;
	int tmpsock;

	tmpsock = socket(AF_INET, SOCK_STREAM, 0);
	if (tmpsock == 1)
		goto end;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // FIXME
	servaddr.sin_port = htons(21370);

	if (connect(tmpsock, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
		goto end;

	_r_dk_socket_fd = tmpsock;

	struct dk_message msg;
	for (;;)
	{
		receive_dk_msg(_r_dk_socket_fd, &msg);
		switch(msg.opcode)
		{
			case R_MSG_INPUT:
				if (_is_inp_hand_attach == 1)
					_cb_input_handle(msg.data, msg.data_alt, msg.data_alt_alt);
				else
					log_warn("Rdevkit input event ignored\n");
				break;
			case R_MSG_SPIERDALAJ:
				log_debug("Closing Rdevkit connection\n");
				goto end;
				break;
		}
	}
end:
	_r_dk_socket_fd = -1;
	log_debug("Rdevkit not found, handler quitting\n");
}

int r_init(SDL_Renderer **renderer, SDL_Window **window, TTF_Font **font, uint64_t flags)
{

	log_debug("Spawning Rdevkit handler thread\n");
	pthread_create(&_r_dk_thread, NULL, _r_devkit_handler, NULL);
	if (flags & ROS_INIT_SDL)
	{
		log_debug("Initializing SDL\n");
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
		{
			log_err("Error initializing SDL:\n%s\n", SDL_GetError());
			exit(1);
		}

		if (SDL_CreateWindowAndRenderer(480, 320, SDL_WINDOW_SHOWN, window, renderer) < 0)
		{
			log_err("Error creating SDL window:\n%s\n", SDL_GetError());
			exit(1);
		}
		if (window == NULL || renderer == NULL)
		{
			log_err("SDL window or renderer are null:\n%s\n", SDL_GetError());
			exit(1);
		}
		SDL_GL_SetSwapInterval(-1);
	}

	if (flags & ROS_INIT_TTF)
	{
		log_debug("Initializing TTF\n");

		if (TTF_Init() < 0)
		{
			log_err("Error initializng TTF:\n%s\n", SDL_GetError());
			exit(1);
		}

		*font = TTF_OpenFont("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", 24);
		if (*font == NULL)
		{
			log_err("Error opening TTF font:\n%s\n", SDL_GetError());
			exit(1);
		}
	}

	if(flags & ROS_INIT_INPUT)
	{
		log_debug("Initializing input handling\n");
		pthread_create(&_r_js_thread, NULL, _r_upd_joystick, NULL);
	}
}

int r_quit(SDL_Renderer *renderer, SDL_Window *window)
{
	send_dk_msg(_r_dk_socket_fd, R_MSG_SPIERDALAJ, 0, 0, 0);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	TTF_Quit();
	SDL_Quit();
	exit(0);
}

