#include <unistd.h>
#include <pthread.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <fcntl.h>
#include "../libragnarok.h"
#include "../librtoolkit.h"

SDL_Event poke;
unsigned int poked;

pthread_t gameListMaker;
pthread_t progListMaker;

struct Item *parents[4];
struct Item *gameHead;
struct Item *progHead;
struct Item *setsHead;
struct Item *selection;

struct stateful settings;

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

int winWidth = 0;
int winHeight = 0;

unsigned int action = 0;
unsigned int canPoke = 0;
unsigned int flushInputEvents = 0;

// regular types
#define ITEM_TYPE_PARENT 0
#define ITEM_TYPE_PROG 1
#define ITEM_TYPE_GAME 2
#define ITEM_TYPE_MUSIC 3
#define ITEM_TYPE_VIDEO 4
// setting types
#define ITEM_TYPE_TOGGLE 5
#define ITEM_TYPE_LIST 6
#define ITEM_TYPE_NUM 7
#define ITEM_TYPE_SUBMENU 8

#define ITEM_TYPE_SPECIAL 2137

struct stateful
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

#ifdef ROS
	#define HOME "/menu/"

	int getBatLevel()
	{
		// TODO: open /sys/class/power_supply/whatever/capacity
		return 80; // 0 - 100
	}

	void do_poweroff()
	{
		system("/sbin/poweroff");
	}

#else
	#define HOME "./menu/"

	void do_poweroff()
	{
		log_debug("Pretending to power off!\n");
		exit(0);
	}

#endif

void poweroff()
{
	log_debug("Powering off!\n");
	do_poweroff();
}

struct Item {
	int type;
	int subtype;
	int id;
	int offsetV; // applies only to parents!
	int targetV; // ditto
	int value;   // applies only to toggle | list | num

	char name[64];
	char mainPath[64];
	char extraPath[64];

	SDL_Rect rect;
	SDL_Texture *texture;

	struct Item *kid; // applies only to submenus
	
	struct listMember *listHead; //applies only to lists
	struct listMember *curValue; //same

	struct Item *next;
	struct Item *prev;
};

struct listMember {
	int id;
	char name[64];
	int val1;
	int val2;
	int val3;

	struct Item *parent;
	struct listMember *next;
};

#include "proglist.h"
#include "gamelist.h"
#include "setslist.h"

void get_text_and_rect(SDL_Renderer *renderer, char *text,
TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect, int r, int g, int b)
{
	int text_width;
	int text_height;
	SDL_Surface *surface;
	SDL_Color textColor = {r, g, b, 0};

	surface = TTF_RenderText_Solid(font, text, textColor);
	*texture = SDL_CreateTextureFromSurface(renderer, surface);
	text_width = surface->w;
	text_height = surface->h;
	SDL_FreeSurface(surface);
	rect->x = 0;
	rect->y = 0;
	rect->w = text_width;
	rect->h = text_height;
}

void *poke_main_thread(void *unused)
{
	while (1)
	{
		if (canPoke == 1)
			SDL_PushEvent(&poke);
		sleep(1);
	}
}

void deinit()
{
	canPoke = 0;

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void loadSettings()
{
	FILE *settingsFile = fopen(HOME"settings.dat", "r");


	if (settingsFile != NULL)
	{
		fseek(settingsFile, 0, SEEK_END);
		if (ftell(settingsFile) == sizeof(struct stateful))
		{
			fseek(settingsFile, 0, SEEK_SET);
			fread(&settings, sizeof(struct stateful), 1, settingsFile);
			fclose(settingsFile);
			return;
		}
	}

	// load default
	settings.r = 255;
	settings.g = 0;
	settings.b = 0;
}

void saveSettings()
{
	FILE *settingsFile = fopen(HOME"settings.dat", "w");
	fwrite(&settings, sizeof(struct stateful), 1, settingsFile);
	fclose(settingsFile);
}

void init()
{
	initSettingsList(0);
	initProgList(1);
	initGameList(2);
}


int clamp(signed int v, int min, int max)
{
	return (v < min)? min : (v > max)? max : v;
}

int abs(signed int input)
{
	return input > 0 ? input : input*-1;
}

void run()
{
	//printf("D: Running %s!\n", selection->mainPath);
	system(selection->mainPath);
}

void trampoline()
{
	switch (selection->type)
	{
		case (ITEM_TYPE_PARENT):
			log_err("wtf __line__\n");
			break;
		/* fall-through */
		case (ITEM_TYPE_PROG):
		case (ITEM_TYPE_GAME):
			canPoke = 0;
			run();
			flushInputEvents = 1;
			canPoke = 1;
			break;
		case (ITEM_TYPE_MUSIC):
			// TODO: switch into music player mode
			break;
		case (ITEM_TYPE_VIDEO):
			// TODO: run hardware accelerated video
			break;
		case (ITEM_TYPE_TOGGLE):
			selection->value = !selection->value;
			break;
		case (ITEM_TYPE_NUM):
			// TODO
			break;
		case (ITEM_TYPE_SUBMENU):
			// TODO
			break;
		default:
			if (strncmp(selection->name, "Poweroff", 3) == 0)
			{
				saveSettings();
				poweroff();
			}

			if (strncmp(selection->name, "ChangeColors", 3) == 0)
			{
				settings.r = random() % 255;
				settings.g = random() % 255;
				settings.b = random() % 255;
			}
	}
}

int handle_input(int type, int code, int value)
{
	if (value == 1 && type == EV_KEY)
	{
		switch (code)
		{

			case BTN_DPAD_UP:
				action = 1;
				break;
			case BTN_DPAD_DOWN:
				action = 2;
				break;
			case BTN_DPAD_RIGHT:
				action = 3;
				break;
			case BTN_DPAD_LEFT:
				action = 4;
				break;
			case BTN_EAST:
				action = 5;
				break;
		}
	}
	return 0;
}

int map(int x, int in_min, int in_max, int out_min, int out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main(void)
{
	pthread_t pinger;
	pinger = pthread_create(&pinger, NULL, poke_main_thread, NULL);

	memset(&poke, 0, sizeof(poke));

	r_init(&renderer, &window, &font, ROS_INIT_SDL | ROS_INIT_TTF | ROS_INIT_INPUT);
	r_attach_input_callback(handle_input);

	unsigned int poked = SDL_RegisterEvents(1);
	poke.type = poked;

	log_debug("Initializing...\n");

	init();
	loadSettings();

	SDL_GetWindowSize(window, &winWidth, &winHeight);

	log_debug("Done initializing, running the main loop\n");

	int batLevel = 0;
	
	batLevel = map(getBatLevel(), 0, 100, 0, 38);
	SDL_Rect batRect;
	SDL_Rect whiteBatRect;

	signed int selectedID = 1;
	int selected = 0;
	SDL_Event event;

	canPoke = 1;

	struct Item *current;
	int currentParentID = 0;
	selection = parents[currentParentID]->next;
	int j = 0;
	
	signed int offsetH = 0;
	signed int targetH = 0;	

	signed int offsetV = 0;
	signed int targetV = 0;
	
	signed int canUP = 0;
	signed int tmp;
	while (1)
	{
		// scroll here, i should multithread it... somehow
		if (offsetH != targetH)
		{
			if (abs(targetH - offsetH) < winWidth/30)
			{
				offsetH = targetH;
			}
			else
			{
				if (targetH < offsetH)
					offsetH -= winWidth/30;
				else
					offsetH += winWidth/30;
			}
		}
		if (offsetV != targetV)
		{
			if (abs(targetV - offsetV) < winHeight/50 /*winHeight/12*/)
			{
				offsetV = targetV;
			}
			else
			{
				if (targetV < offsetV)
					offsetV -= winHeight/38 /*(winHeight-(2+winWidth/20)/49)*/;
				else
					offsetV += winHeight/38 /*(winHeight-(2+winWidth/20)/49)*/;
			}
		}
		// end scroll code
		SDL_RenderClear(renderer);
		for (int i = 0; i < 3; i++)
		{
			current = parents[i]->next;
			j = 1;
			while (1)
			{
				current->rect.y = j * (winWidth/20) + (j == 0 ? 0 : 5) + ( j == 0 ? 0 : offsetV);
				current->rect.x = i * (winWidth/3) + offsetH;

				tmp = 300 - 255*((float)current->rect.x / (winWidth/3));

				SDL_SetTextureAlphaMod(current->texture, clamp(tmp, 0, 255));
				if (currentParentID == i && selectedID == j)
					SDL_SetTextureColorMod(current->texture, settings.r, settings.g, settings.b);
				else
					SDL_SetTextureColorMod(current->texture, 255, 255, 255);
				SDL_RenderCopy(renderer, current->texture, NULL, &current->rect);

				if (current->next == NULL)
				{

					if (currentParentID == i)
					{
						if (selectedID == j)
							canUP = 0;
						else
							canUP = 1;
					}
					break;
				}

				j++;
				current = current->next;
			}
			parents[i]->rect.y = 0;
			parents[i]->rect.x = i * (winWidth/3) + offsetH;


			SDL_SetTextureAlphaMod(parents[i]->texture, clamp(tmp, 0, 255));
			tmp = parents[i]->rect.w;
			parents[i]->rect.w = (winWidth/3);
			SDL_RenderFillRect(renderer, &parents[i]->rect);
			parents[i]->rect.w = tmp;
			SDL_RenderCopy(renderer, parents[i]->texture, NULL, &parents[i]->rect);
			
		}

		batRect.x = (winWidth-38);
		batRect.y = 5;
		batRect.w = 38 - batLevel;
		batRect.h = parents[0]->rect.h - 7;
	
		whiteBatRect.x = (winWidth-40);
		whiteBatRect.y = 4;
		whiteBatRect.w = 42;
		whiteBatRect.h = parents[0]->rect.h - 5;

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, 0, parents[0]->rect.h, winWidth, parents[0]->rect.h);

		SDL_RenderDrawLine(renderer, winWidth-43, parents[0]->rect.h-10, winWidth-43, 10);
		SDL_RenderDrawLine(renderer, winWidth-42, parents[0]->rect.h-10, winWidth-42, 10);
		SDL_RenderDrawLine(renderer, winWidth-41, parents[0]->rect.h-10, winWidth-41, 10);

		SDL_RenderFillRect(renderer, &whiteBatRect);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		
		SDL_RenderFillRect(renderer, &batRect);


		SDL_RenderPresent(renderer);

		while (SDL_PollEvent(&event) == 1)
		{
			if (event.type == poked)
			{
				// TODO: UPDATE DATA
				//log_debug("Responding to poke from poker");
				batLevel = map(getBatLevel(), 0, 100, 1, 38);
				;;
			}
			switch (event.type)
			{
				case SDL_QUIT:
					goto out;
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
						case SDL_WINDOWEVENT_RESIZED:
							winHeight = event.window.data2;
							winWidth = event.window.data1;
							break;
					}

			}
		}
		switch(action)
		{
			case 1:
				if (selectedID > 1)
				{
					selectedID--; // 1 is on top of 2
					selection = selection->prev;
				}
				if (selectedID > 5)
				{
					targetV += winWidth/20;
				}
				break;
			case 2:
				if (canUP == 1)
				{
					if (selectedID > 5)
					{
						targetV -= winWidth/20;
					}

					selectedID++;
					selection = selection->next;
				}
				break;
			case 3:
				if (currentParentID < 2)
				{
					selectedID = 1;
					targetV = 0;
					targetH -= winWidth/3;
					currentParentID++;
					selection = parents[currentParentID]->next;
				}
				break;
			case 4:
				if (currentParentID > 0)
				{
					selectedID = 1;
					targetV = 0;
					targetH += winWidth/3;
					currentParentID--;
					selection = parents[currentParentID]->next;
				}
				break;
			case 5:
				trampoline();
				break;
		}
		action = 0;
	}

out:
	canPoke = 0;


	r_quit(window, renderer);
	return 0;
}
