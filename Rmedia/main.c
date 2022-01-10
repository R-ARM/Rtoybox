#include "../libragnarok.h"
#include <dirent.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

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

int handle_input(int type, int code, int value)
{
	// stub
	return 0;
}

struct track {
	char path[256];
	char name[256];

	SDL_Rect rect;
	SDL_Texture *texture;

	struct track *next;
	struct track *prev;
};

struct track *trackHead;

struct randomText {
	char content[256];
	SDL_Rect rect;
	SDL_Texture *texture;
};

struct randomText *filenames;

void push(struct track *head, char *data)
{
	struct track *new;
	new = malloc(sizeof(struct track));
	strncpy(new->path, data, 255);
	new->next = head->next;
	head->next = new;
}

int howManyFiles = 0;

int main(int argc, char *argv[])
{
	r_init(&renderer, &window, &font, ROS_INIT_SDL | ROS_INIT_TTF | ROS_INIT_INPUT);
	r_attach_input_callback(handle_input);

	trackHead = malloc(sizeof(struct track));


	filenames = malloc(sizeof(struct randomText));
	strcpy(filenames->content, "Filenames");
	get_text_and_rect(renderer, filenames->content, font, &filenames->texture, &filenames->rect, 255, 255, 255);
	filenames->rect.x = 0;
	filenames->rect.y = 0;

	srand(time(0));

	DIR *d;
	struct dirent *dir;
	chdir(argv[1]);
	d = opendir(".");
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (dir->d_type == DT_REG)
			{
				log_debug("Loading %s\n", realpath(dir->d_name, NULL));
				push(trackHead, realpath(dir->d_name, NULL));
				
				get_text_and_rect(renderer, trackHead->next->path, font, &trackHead->next->texture, &trackHead->next->rect, 255, 255, 255);
				howManyFiles++;
			}
		}
		closedir(d);
	}


	struct track *cur;
	int i = 0;


	SDL_Event event;
	while (1)
	{
		while(SDL_PollEvent(&event) == 1)
		{
			switch(event.type)
			{
				case SDL_QUIT:
					r_quit(renderer, window);
					break;
			}
		}


		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		
		cur = trackHead->next;
		i = 0;
		while(i < howManyFiles)
		{

			cur->rect.x = 0;
			cur->rect.y = (i+1) * 22;

			SDL_RenderCopy(renderer, cur->texture, NULL, &cur->rect);

			i++;
			cur = cur->next;
		}


		SDL_RenderCopy(renderer, filenames->texture, NULL, &filenames->rect);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderDrawLine(renderer, 0, 23, 480, 23);

		SDL_RenderPresent(renderer);
	}
}
