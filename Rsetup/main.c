#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>
#include <dirent.h>
#include <errno.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

void buttonStateCallback(struct r_tk_btn *btn)
{
	if(strncmp(btn->name, "Internal", 4) == 0)
	{
		system("/bin/rsetup-internal");
		r_tk_next_tab(toolkit);
	}
	else if(strncmp(btn->name, "External", 4) == 0)
	{
		system("/bin/rsetup-external");
		r_tk_next_tab(toolkit);
	}
	else if(strncmp(btn->name, "exFAT", 4) == 0)
	{
		system("/bin/rsetup-exfat");
		system("/sbin/reboot");
	}
	else if(strncmp(btn->name, "ext4", 4) == 0)
	{
		system("/bin/rsetup-ext4");
		system("/sbin/reboot");
	}
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	toolkit = new_r_tk(&window, &renderer, &font, "Data Location", buttonStateCallback);
	r_attach_input_callback(_r_tk_input_handler);
	toolkit->inputTabSwitching = 0;

	new_btn(toolkit, toolkit->tabHead, "Internal SD Card", 0, 0);
	new_btn(toolkit, toolkit->tabHead, "External SD Card", 0, 0);
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "Data Partition Filesystem");
	new_btn(toolkit, toolkit->tabHead, "exFAT", 0, 0);
	new_btn(toolkit, toolkit->tabHead, "ext4", 0, 0);
	toolkit->tabHead->isList = 1;

	SDL_Event event;
	while (1)
	{
		r_tk_draw(toolkit);

		fflush(stdout);
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
