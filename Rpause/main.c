#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>
#include <dirent.h>
#include <errno.h>
#include <alsa/asoundlib.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

int readIntFrom(char *path)
{
	FILE *fd = fopen(path, "r");
	int tmp = 0;
	fscanf(fd, "%d", &tmp);
	close(fd);
	return tmp;
}

int pid;

void buttonStateCallback(struct r_tk_btn *btn)
{
	struct btnData *tmp;
	log_debug("button %s state %d\n", btn->name, btn->state);
	if(strncmp(btn->name, "Resume", 4) == 0)
	{
		kill(pid, 18); // SIGCONT
		exit(0);
	} else if(strncmp(btn->name, "Exit", 4) == 0)
	{
		kill(pid, 18); // SIGCONT
		//msleep(500);
		kill(pid, 15); // SIGINT
		exit(0);
	} else if(strncmp(btn->name, "Toggle FPS View", 4) == 0)
	{
		kill(pid, 18); // SIGCONT
		kill(pid, 10); // SIGUSR1
		exit(0);
	}
}

#define ACT_NEXT_BTN	1
#define ACT_PREV_BTN	2
#define ACT_ACT 	4

int action = 0;
int handle_input(int type, int code, int value)
{
	if(value != 1) return 0;
	switch(code)
	{
		case BTN_DPAD_UP:
			action = ACT_NEXT_BTN;
			break;
		case BTN_DPAD_DOWN:
			action = ACT_PREV_BTN;
			break;
		case BTN_EAST:
			action = ACT_ACT;
			break;
	}
	return 0;
}

int getVolume()
{
	int pid = fork();
	if(pid > 0)
	{
		int status;
		waitpid(pid, &status, 0);
		return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	}
	else
	{
#ifdef ROS
		execl("/bin/rpause-getvol", "/bin/rpause-getvol", NULL);
#else
		execl("./volhelper.sh", "./volhelper.sh", NULL);
#endif
	}
}

int getBrightness()
{
	int cur = readIntFrom("/sys/class/backlight/backlight/brightness");
	int max = readIntFrom("/sys/class/backlight/backlight/max_brightness");

	return cur * (100.0/max);
}

int getBatPercent()
{
#ifdef ROS
	int cur = readIntFrom("/sys/class/power_supply/rk817-battery/charge_now");
	int max = readIntFrom("/sys/class/power_supply/rk817-battery/charge_full_design");
	return cur * (100.0/max);
#else
	//return readIntFrom("/sys/class/power_supply/rk817-battery/capacity");
#endif
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	r_attach_input_callback(handle_input);
	toolkit = new_r_tk(&window, &renderer, &font, "System", buttonStateCallback);
	new_btn_list_batch(toolkit, toolkit->tabHead, 5, "Resume", "Toggle FPS View", "Volume", "Brightness", "Battery");
	toolkit->tabHead->isList = 1;
	toolkit->tabHead->scrolling = 0;

	char volBuffer[5] = "    ";
	char brBuffer[5] = "    ";
	char batBuffer[5] = "    ";

	snprintf(volBuffer, 4, "%d%%", getVolume());
	snprintf(brBuffer, 4, "%d%%", getBrightness());
	snprintf(batBuffer, 4, "%d%%", getBatPercent());

	new_cotab(toolkit, toolkit->tabHead, 200);
	new_btn_list_batch(toolkit, toolkit->tabHead->coTab, 4, " ", " ", volBuffer, brBuffer, batBuffer);
	toolkit->tabHead->coTab->isList = 1;
	toolkit->tabHead->coTab->scrolling = 0;

	pid = readIntFrom("/tmp/curpid");
	kill(pid, 19); // SIGSTOP

	SDL_Event event;
	while (1)
	{
		SDL_RenderClear(renderer);
		r_tk_draw(toolkit, 480);
		SDL_RenderPresent(renderer);

		fflush(stdout);
		switch(action)
		{
			case ACT_NEXT_BTN:
				r_tk_next_btn(toolkit);
				break;
			case ACT_PREV_BTN:
				r_tk_prev_btn(toolkit);
				break;
			case ACT_ACT:
				r_tk_action(toolkit);
				break;
		}
		action = 0;
		while(SDL_PollEvent(&event) == 1)
		{
			switch(event.type)
			{
				case SDL_QUIT:
					exit(0);
					break;
			}
		}
	}
}
