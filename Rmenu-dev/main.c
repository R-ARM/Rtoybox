#include "../libragnarok.h""

int clamp(signed int v, int min, int max)
{
	return (v < min)? min : (v > max)? max : v;
}

int map(int x, int in_min, int in_max, int out_min, int out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

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

struct r_tk_tab
{
	int id;
	
	char name[255];
	SDL_Rect rect;
	SDL_Texture *text;

	struct r_tk_tab *next;
	struct r_tk_tab *prev;
};

struct r_tk
{
	SDL_Window **window;
	SDL_Renderer **renderer;
	TTF_Font **font;

	int winX;
	int winY;

	struct r_tk_tab *curTab;

	struct r_tk_tab *tabHead;
	struct r_tk_tab *tabTail;
};

void r_tk_next_tab(struct r_tk *tk)
{
	tk->curTab = tk->curTab->next;
};

void r_tk_prev_tab(struct r_tk *tk)
{
	tk->curTab = tk->curTab->prev;
};

void new_tab(struct r_tk *tk, char *name)
{
	struct r_tk_tab *tmp;

	tmp = malloc(sizeof(struct r_tk_tab));
	
	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	tmp->id = tk->tabTail->id + 1;
	strcpy(tmp->name, name);

	tmp->prev = tk->tabHead;
	tmp->next = tk->tabTail;

	tk->tabTail->prev = tmp;
	tk->tabTail = tmp;

	tk->tabTail->prev = tk->tabHead;
	tk->tabHead->next = tk->tabTail;

	tk->curTab = tmp;
}

struct r_tk * new_r_tk(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font, char* initTabName)
{
	struct r_tk *tmp;

	tmp = malloc(sizeof(struct r_tk));
	tmp->window = window;
	tmp->renderer = *renderer;
	tmp->font = font;

	struct r_tk_tab *initialTab;
	initialTab = malloc(sizeof(struct r_tk_tab));
	initialTab->id = 0;
	initialTab->next = initialTab;
	initialTab->prev = initialTab;
	strcpy(initialTab->name, initTabName);

	get_text_and_rect(*renderer, "Test", *font, &initialTab->text, &initialTab->rect, 255, 255, 255);

	tmp->tabHead = initialTab;
	tmp->tabTail = initialTab;
	tmp->curTab = initialTab;

	return tmp;
}

int r_tk_draw(struct r_tk *tk)
{
	// draw tabs
	struct r_tk_tab *tmp;
	int i = 0;

	tmp = tk->tabHead;
	while(1)
	{
		tmp->rect.x = i;
		i += tmp->rect.w + 10; // TODO: screen size scale
		if(tmp == tk->curTab)
			SDL_SetTextureColorMod(tmp->text, 255, 0, 0);
		else
			SDL_SetTextureColorMod(tmp->text, 255, 255, 255);
		SDL_RenderCopy(tk->renderer, tmp->text, NULL, &tmp->rect);
		if(tmp->next == tk->tabHead)
			break;

		tmp = tmp->next;
	}
	// line separating tabs and other widgets
	SDL_SetRenderDrawColor(tk->renderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(tk->renderer, 0, 25, 480, 25);
	SDL_SetRenderDrawColor(tk->renderer, 0, 0, 0, 255);
}

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);

	struct r_tk *toolkit;
	toolkit = new_r_tk(&window, &renderer, &font, "Test");
	new_tab(toolkit, "two");
	new_tab(toolkit, "3");
	new_tab(toolkit, "four");

	SDL_Event event;
	while (1)
	{
		SDL_RenderClear(renderer);
		r_tk_draw(toolkit);
		SDL_RenderPresent(renderer);

		while (SDL_PollEvent(&event) == 1)
		{
			switch (event.type)
			{
				case SDL_QUIT:
					exit(0);
					break;
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym)
					{
						case SDLK_RIGHT:
							r_tk_next_tab(toolkit);
							break;
						case SDLK_LEFT:
							r_tk_prev_tab(toolkit);
					}
					break;
			}
		}
	}

	//r_quit();
}
