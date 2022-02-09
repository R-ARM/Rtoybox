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

void buttonStateCallback(struct r_tk_btn *btn)
{
	struct btnData *tmp;
	log_debug("button %s state %d\n", btn->name, btn->state);
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
		execl("./volhelper.sh", "./volhelper.sh", NULL);
	}
}

int getBrightness()
{
	FILE *curfd = fopen("/sys/class/backlight/intel_backlight/brightness", "r");
	int cur = 0;
	fscanf(curfd, "%d", &cur);
	close(curfd);

	FILE *maxfd = fopen("/sys/class/backlight/intel_backlight/max_brightness", "r");
	int max = 0;
	fscanf(maxfd, "%d", &max);
	close(maxfd);

	return cur * (100.0/max);
}

int getBatPercent()
{
	FILE *infd = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	int val = 0;
	fscanf(infd, "%d", &val);
	close(infd);

	return val;
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	r_attach_input_callback(handle_input);
	toolkit = new_r_tk(&window, &renderer, &font, "System", buttonStateCallback);
	new_btn_list_batch(toolkit, toolkit->tabHead, 5, "Resume", "Exit", "Volume", "Brightness", "Battery");
	toolkit->tabHead->isList = 1;
	toolkit->tabHead->scrolling = 0;

	char volBuffer[5] = "    ";
	char brBuffer[5] = "    ";
	char batBuffer[5] = "    ";

	snprintf(volBuffer, 4, "%d%%", getVolume());
	snprintf(brBuffer, 4, "%d%%", getBrightness());
	snprintf(batBuffer, 4, "%d%%", getBatPercent());

	new_cotab(toolkit, toolkit->tabHead, 200);
	new_btn_list_batch(toolkit, toolkit->tabHead->coTab, 5, " ", " ", volBuffer, brBuffer, batBuffer);
	toolkit->tabHead->coTab->isList = 1;
	toolkit->tabHead->coTab->scrolling = 0;

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
