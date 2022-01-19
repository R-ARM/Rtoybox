#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>
#include <dirent.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

void buttonStateCallback(struct r_tk_btn *btn)
{
	printf("button %s state %d\n", btn->name, btn->state);
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	toolkit = new_r_tk(&window, &renderer, &font, "Tracks", buttonStateCallback);
	DIR *d;
	struct dirent *ent;
	d = opendir("./music");
	if(d)
	{
		while((ent = readdir(d)) != NULL)
		{
			if(ent->d_type == DT_REG)
			{
				log_debug("Loading %s\n", ent->d_name);
				new_btn(toolkit, toolkit->tabHead, ent->d_name, 0, 0);
			}
		}
	}
	else
	{
		log_err("Failed opening directory\n");
		exit(1);
	}

	toolkit->tabHead->isList = 1;

	SDL_Event event;
	while (1)
	{
		SDL_RenderClear(renderer);
		r_tk_draw(toolkit, 480);
		SDL_RenderPresent(renderer);

		while(SDL_PollEvent(&event) == 1)
		{
			switch(event.type)
			{
				case SDL_QUIT:
					exit(0);
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym)
					{
					case SDLK_w:
						r_tk_next_tab(toolkit);
						break;
					case SDLK_q:
						r_tk_prev_tab(toolkit);
						break;
					case SDLK_a:
					case SDLK_s:
						r_tk_toggle_cotab(toolkit);
						break;
					case SDLK_z:
						r_tk_prev_btn(toolkit);
						break;
					case SDLK_x:
						r_tk_next_btn(toolkit);
						break;
					case SDLK_e:
						r_tk_action(toolkit);
						break;
					}
			}
		}
	}
}
