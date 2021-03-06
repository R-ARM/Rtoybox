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
	char *emu;
	char *arg;
	char *path;
	enum btnType type;
};

// TODO: make variadic
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
		free(pidFile);

		r_flush_input_events();
	}
	else
	{
		setenv("GALLIUM_HUD", "fps", 1);
		setenv("GALLIUM_HUD_TOGGLE_SIGNAL", "10", 1);
		setenv("GALLIUM_HUD_VISIBLE", "false", 1);

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

void writeMednafenConfigLine(char* mod, char *opt, char* value)
{
	pid_t childPid;

	// mednafen config changes need argument "-key" instead "key" itself
	char *key = malloc(strlen(mod) + strlen(opt) + 2);
	sprintf(key, "-%s.%s", mod, opt);

	log_debug("Setting \"%s\" to \"%s\"\n", key, value);

	childPid = fork();
	if(childPid > 0)
	{
		int status;
		waitpid(childPid, &status, 0);
	}
	else
	{
		execl("/usr/bin/mednafen", "mednafen", key, value, NULL);
		log_err("Error changing mednafen config: %s\n", strerror(errno));
		exit(1);
	}
}

inline void writeMednafenConfigLineInt(char *mod, char *opt, int value)
{
	char buf[65];
	sprintf(buf, "%d", value);

	writeMednafenConfigLine(mod, opt, buf);
}

void buttonStateCallback(struct r_tk_btn *btn)
{
	struct btnData *tmp;
	if(btn->type == BTN_TYPE_ONEOF_CHILD)
	{
		writeMednafenConfigLine((char *)toolkit->curTab->progData, (char *)toolkit->curTab->curCoTab->progData, (char *)btn->progData);
	}
	else if(strncmp(btn->name, "Power Off", 5) == 0)
	{
		log_debug("Powering Off!");
#ifdef ROS
		run_wait("/sbin/poweroff", "", "");
#else
		exit(0);
#endif
	}
	else if(btn->progData != NULL)
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

	char name[256];
	char path[256];

	while(fscanf(in, "%s %s", name, path) == 2)
	{
		log_debug("Got program: name: \"%s\", path: \"%s\"\n", name, path);

		tmpData = malloc(sizeof(struct btnData));
		tmpData->type = prog;
		tmpData->path = malloc(strlen(path));
		strcpy(tmpData->path, path);

		new_btn(tk, tk->tabHead, name, 0, 0);
		tk->tabHead->btnTail->progData = tmpData;
	}
}

int loadPackageData(struct r_tk *tk)
{
}

struct emulator {
	struct r_tk *tk;
	char* ext;
	char* cmd;
	char* system;
	char* args;
	char* mednafen_module;
};

void* loadRomList(void *arg);
int loadEmulators(struct r_tk *tk)
{
	char cmd[256] = "";
	char system[256] = "";
	char args[256] = "";
	char ext[256] = "";
	char tmp[256] = "";
	char med_mod[256] = "";
	struct emulator *emu_tmp;

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
	while(feof(emus) == 0)
	{
		fgets(tmp, 256, emus);
		tmp[strcspn(tmp, "\n")] = '\0';	// get rid of newline at the end
		tmp[strcspn(tmp, "]")] = '\0';	// and of the system name end marker

		if(strncmp("[", tmp, 1) == 0)
		{
			log_debug("Got config entry: command \"%s\", system \"%s\", ext \"%s\", args \"%s\", mednafen module \"%s\"\n", cmd, system, ext, args, med_mod);
			emu_tmp = malloc(sizeof(struct emulator));
			emu_tmp->tk = tk;
			emu_tmp->ext = ext;
			emu_tmp->cmd = cmd;
			emu_tmp->system = system;
			emu_tmp->args = args;
			emu_tmp->mednafen_module = med_mod;
			loadRomList(emu_tmp);
			strncpy(system, &tmp[1], 255-2);

			strcpy(cmd, "");	// don't carry over old values
			strcpy(args, "");
			strcpy(ext, "");
			strcpy(med_mod, "");
		}
		else if(strncmp("command", tmp, 7) == 0)
			strncpy(cmd, &tmp[8], 255-7);
		else if(strncmp("ext", tmp, 3) == 0)
			strncpy(ext, &tmp[4], 255-3);
		else if(strncmp("args", tmp, 4) == 0)
			strncpy(args, &tmp[5], 255-4);
		else if(strncmp("mednafen_module", tmp, 15) == 0)
			strncpy(med_mod, &tmp[16], 255-15);
		else if(strlen(tmp) != 0)	// ignore empty lines
			log_err("Malformed option \"%s\"\n", tmp);
	}
}


void* loadRomList(void *arg)
{
	struct emulator *input = (struct emulator *)arg;
	char romdir[256] = "";
	char fancyName[256] = "";
#ifdef ROS
	strcpy(romdir, "/data/Roms/");
#else
	strcpy(romdir, "./Roms/");
#endif
	strcat(romdir, input->system);
	strcat(romdir, "/");
	log_debug("Looking for \"%s\" roms in \"%s\"\n", input->system, romdir);
	int i = 0;

	struct btnData *tmp;
	DIR *d;
	struct dirent *ent;
	d = opendir(romdir);
	if(d)
	{
		while((ent = readdir(d)) != NULL)
		{
			if(ent->d_type == DT_REG)
			{
				if(i == 0)
				{
					struct r_tk_tab *tab = new_tab(input->tk, input->system);
					tab->progData = malloc(strlen(input->mednafen_module) + 1);
					strcpy(tab->progData, input->mednafen_module);

					if(input->mednafen_module != 0 && strlen(input->mednafen_module) > 0)
					{
						struct r_tk_btn *oneof = new_oneof(input->tk, input->tk->tabHead, "Stretching options", 0, 0, 0);
						new_oneof_opt(input->tk, oneof, "Fullscreen", "full");
						new_oneof_opt(input->tk, oneof, "Keep aspect ratio", "aspect");
						new_oneof_opt(input->tk, oneof, "Integer scaling", "aspect_int");

						oneof->coTab->progData = (char *)"stretch";
					}
				}
				strncpy(fancyName, ent->d_name, strcspn(ent->d_name, "."));
				new_btn(input->tk, input->tk->tabHead, fancyName, 0, 0);
				tmp = malloc(sizeof(struct btnData));
				tmp->type = rom;

				tmp->emu = malloc(strlen(input->cmd));
				strcpy(tmp->emu, input->cmd);
				
				tmp->arg = malloc(strlen(input->args));
				strcpy(tmp->arg, input->args);

				tmp->path = malloc(strlen(romdir) + strlen(ent->d_name));
				sprintf(tmp->path, "%s%s", romdir, ent->d_name);
				input->tk->tabHead->btnTail->progData = tmp;
				i++;
			}
		}
		input->tk->tabHead->isList = 1;
		free(d);
	}
	else
		log_warn("Failed opening \"%s\" directory\n", romdir);
	log_debug("Found \"%d\" roms for \"%s\" in \"%s\" using mednafen module \"%s\"\n", i, input->system, romdir, input->mednafen_module);
	free(input);
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	toolkit = new_r_tk(&window, &renderer, &font, "System", buttonStateCallback);

	r_attach_input_callback(_r_tk_input_handler);
	new_btn_list_batch(toolkit, toolkit->tabHead, 3, "Power Off", "Update", "USB Mode");
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "Ragnarok Programs");
	loadStaticData(toolkit);
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "Ports");
	loadPackageData(toolkit);
	toolkit->tabHead->isList = 1;

	loadEmulators(toolkit);
	
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
