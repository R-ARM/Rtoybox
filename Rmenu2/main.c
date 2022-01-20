#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>
#include <dirent.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

#define TYPE_ROM	1
#define TYPE_PROG	2
#define TYPE_SPECIAL	3

struct btnData
{
	int type;
	char emu[256];
	char path[256];
};

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
	{
		log_err("Error reading program list file\n");
		return 1;
	}

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

int loadEmulators(struct r_tk *tk)
{
	char cmd[256];
	char system[256];
	char args[256];
	char ext[256];

	log_debug("Loading emulator config file\n");

	FILE *emus = fopen("emulators", "r");
	if(!emus)
	{
		log_err("Failed to open emulator config file\n");
		return 1;
	}
	while(1)
	{
		fscanf(emus, "{\ncommand %s\nsystem %s\next %s\nargs %s\n}\n", cmd, system, ext, args);

		if(strncmp("__none__", args, 8) == 0)
			args[0] = '\0';
		log_debug("Got config entry: command %s, system %s, ext %s, args %s\n", cmd, system, ext, args);
		loadRomList(tk, ext, cmd, system);
		
		if(feof(emus) != 0)
			break;
	}
}


int loadRomList(struct r_tk *tk, char *ext, char* emu, char* system)
{
	char temp[256];
	char fancyName[256];
	strcpy(temp, "./roms/");
	strcat(temp, system);
	strcat(temp, "/");
	log_debug("Looking for %s roms in %s\n", system, temp);
	int i = 0;

	struct btnData *tmp;
	DIR *d;
	struct dirent *ent;
	d = opendir(temp);
	if(d)
	{
		while((ent = readdir(d)) != NULL)
		{
			if(ent->d_type == DT_REG)
			{
				if(i == 0)
					new_tab(tk, system);
				strncpy(fancyName, ent->d_name, strlen(ent->d_name) - (1+strlen(ext)));
				new_btn(tk, tk->tabHead, fancyName, 0, 0);
				tmp = malloc(sizeof(struct btnData));
				strcpy(tmp->emu, emu);
				strcpy(tmp->path, ent->d_name);
				tk->tabHead->btnTail->progData = tmp;
				i++;
			}
		}
		toolkit->tabHead->isList = 1;
	}
	else
	{
		log_warn("Failed opening %s directory\n", temp);
		return 1;
	}
	if(i == 0)
		log_debug("No %s roms found in %s\n", system, temp);
	else
		log_debug("Found %d roms for %s\n", i, system);
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

	new_tab(toolkit, "Ports");
	loadPackageData(toolkit);
	toolkit->tabHead->isList = 1;

	loadEmulators(toolkit);
	//loadRomList(toolkit, "gba", "mgba", "GBA");
	
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
