#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

void run_wait(char *path)
{
	pid_t pidKaszojada;
	log_debug("Running %s\n", path);
	
	pidKaszojada = fork();
	if(pidKaszojada > 0)
	{
		int status;
		waitpid(pidKaszojada, &status, 0);
		r_flush_input_events();
	}
	else
	{
		execl(path, (char *)NULL);
		exit(1);
	}
}

void buttonStateCallback(struct r_tk_btn *btn)
{
	log_debug("button %s state %d\n", btn->name, btn->state);
	log_debug("prog data: %s\n", (char *)btn->progData);
	if((char *) btn->progData != NULL)
		run_wait((char *)btn->progData);
}

int loadStaticData(struct r_tk *tk)
{
	FILE *in = fopen("programs", "r");
	if(!in)
		log_err("Error reading program list file\n");

	char tmp[256];
	char tmp2[256];

	while(1)
	{
		fscanf(in, "%s %s", tmp, tmp2);

		if(feof(in) != 0)
			break;

		new_btn(tk, tk->tabHead, tmp, 0, 0);
		tk->tabHead->btnTail->progData = malloc(strlen(tmp2)+1);
		strcpy(tk->tabHead->btnTail->progData, tmp2);

	}
}

int loadPackageData(struct r_tk *tk)
{
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	toolkit = new_r_tk(&window, &renderer, &font, "System", buttonStateCallback);
	new_btn_list_batch(toolkit, toolkit->tabHead, 3, "Power Off", "Update", "USB Mode");
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "Ragnarok Programs");
	loadStaticData(toolkit);
	toolkit->tabHead->isList = 1;


	new_tab(toolkit, "Games");
	loadPackageData(toolkit);
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "qwertyuiop");
	new_tab(toolkit, "asdfghjkl");
	new_tab(toolkit, "zxcvbnm");
	
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
