#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>
#include <dirent.h>
#include <errno.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

enum btnType{rom, prog, special};

struct btnData
{
	enum btnType type;
	char emu[256];
	char path[256];
	char arg[256];
};

void run_wait(char *path, char *arg1, char *arg2)
{
	pid_t pidKaszojada;
	log_debug("Running %s %s %s\n", path, arg1, arg2);
	
	pidKaszojada = fork();
	if(pidKaszojada > 0)
	{
		int status;
		FILE *pidFile = fopen("/tmp/curpid", "w");
		fprintf(pidFile, "%d", pidKaszojada);
		fclose(pidFile);

		waitpid(pidKaszojada, &status, 0);

		// re-open and clear out
		fopen("/tmp/curpid", "w");
		fprintf(pidFile, "0");
		fclose(fopen("/tmp/curPid", "w"));

		r_flush_input_events();
	}
	else
	{
		if(strlen(arg2) > 0)
			execl(path, path, arg1, arg2, NULL);
		else
			if(strlen(arg1) > 0)			
				execl(path, path, arg1, NULL);
			else
				execl(path, path, NULL);
		log_err("Error running %s: %s\n", path, strerror(errno));
		exit(1);
	}
}

void buttonStateCallback(struct r_tk_btn *btn)
{
	struct btnData *tmp;
	log_debug("button %s state %d\n", btn->name, btn->state);
	log_debug("prog data: %s\n", (char *)btn->progData);
	if(btn->progData != NULL)
	{
		tmp = (struct btnData *)btn->progData;
		switch(tmp->type)
		{
			case prog:
				run_wait(tmp->path, "", "");
				break;
			case rom:
				run_wait(tmp->emu, tmp->path, tmp->arg);
				break;
		}
	}
	else if(strncmp(btn->name, "Power Off", 9) == 0)
	{
		log_debug("Powering Off!");
#ifdef ROS
		run_wait("/sbin/poweroff", "", "");
#else
		exit(0);
#endif
	}
}

int loadStaticData(struct r_tk *tk)
{
	struct btnData *tmpData;
#ifdef ROS
	FILE *in = fopen("/etc/rmenu/programs", "r");
#else
	FILE *in = fopen("programs", "r");
#endif
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
		tmpData = malloc(sizeof(struct btnData));
		tmpData->type = prog;
		strcpy(tmpData->path, tmp2);

		tk->tabHead->btnTail->progData = tmpData;
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

#ifdef ROS
	FILE *emus = fopen("/etc/rmenu/emulators", "r");
#else
	FILE *emus = fopen("emulators", "r");
#endif
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
		loadRomList(tk, ext, cmd, system, args);
		
		if(feof(emus) != 0)
			break;
	}
}


int loadRomList(struct r_tk *tk, char *ext, char* emu, char* system, char* args)
{
	char temp[256];
	char fancyName[256];
	char fullPath[256];
#ifdef ROS
	strcpy(temp, "/roms/");
#else
	strcpy(temp, "./roms/");
#endif
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
//				if(!!(strcmp(strrchr(ent->d_name, '\0') - 3, ext)))
//					continue; // wrong extension

				if(i == 0)
					new_tab(tk, system);
//				strncpy(fancyName, ent->d_name, strlen(ent->d_name) - (1+strlen(ext)));
//				new_btn(tk, tk->tabHead, fancyName, 0, 0);
				new_btn(tk, tk->tabHead, ent->d_name, 0, 0);
				tmp = malloc(sizeof(struct btnData));
				tmp->type = rom;
				strcpy(tmp->emu, emu);
				strcpy(fullPath, temp);
				strcpy(tmp->arg, args);
				strcpy(tmp->path, strcat(fullPath, ent->d_name));
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

int action = 0;

#define ACT_NEXT_BTN	1
#define ACT_PREV_BTN	2
#define ACT_TOG_CTAB	3
#define ACT_ACT 	4
#define ACT_NEXT_TAB	5
#define ACT_PREV_TAB	6

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
		case BTN_DPAD_RIGHT:
		case BTN_DPAD_LEFT:
			action = ACT_TOG_CTAB;
			break;
		case BTN_EAST:
			action = ACT_ACT;
			break;
		case BTN_TR:
			action = ACT_NEXT_TAB;
			break;
		case BTN_TL:
			action = ACT_PREV_TAB;
	}
	return 0;
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	r_attach_input_callback(handle_input);
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

		fflush(stdout);
		switch(action)
		{
			case ACT_NEXT_BTN:
				r_tk_next_btn(toolkit);
				break;
			case ACT_PREV_BTN:
				r_tk_prev_btn(toolkit);
				break;
			case ACT_TOG_CTAB:
				r_tk_toggle_cotab(toolkit);
				break;
			case ACT_ACT:
				r_tk_action(toolkit);
				break;
			case ACT_NEXT_TAB:
				r_tk_next_tab(toolkit);
				break;
			case ACT_PREV_TAB:
				r_tk_prev_tab(toolkit);
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
