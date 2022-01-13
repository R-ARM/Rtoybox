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

struct r_tk_btn
{
	int id;
	int walked;
	char name[255];
	SDL_Rect rect;
	SDL_Texture *text;

	struct r_tk_btn *next;
};

struct r_tk_tab
{
	int id;
	
	char name[255];
	SDL_Rect rect;
	SDL_Texture *text;

	int hasButtons;
	struct r_tk_btn *curBtn;
	struct r_tk_btn *btnHead;

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
}

void r_tk_prev_tab(struct r_tk *tk)
{
	tk->curTab = tk->curTab->prev;
}

void r_tk_next_btn(struct r_tk *tk)
{
	if(tk->curTab->curBtn->next)
		tk->curTab->curBtn = tk->curTab->curBtn->next;
}

void new_tab(struct r_tk *tk, char *name)
{
	struct r_tk_tab *tmp;

	tmp = malloc(sizeof(struct r_tk_tab));
	
	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strcpy(tmp->name, name);


	tmp->id = tk->tabTail->id + 1;
	tmp->hasButtons = 0;

	tmp->prev = tk->tabHead;
	tmp->next = tk->tabTail;

	tk->tabTail->prev = tmp;
	tk->tabTail = tmp;

	tk->tabTail->prev = tk->tabHead;
	tk->tabHead->next = tk->tabTail;

	tk->curTab = tmp;
}

void new_btn(struct r_tk *tk, char *name, int x, int y)
{
	struct r_tk_btn *tmp;

	tmp = malloc(sizeof(struct r_tk_btn));

	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strcpy(tmp->name, name);

	tmp->rect.x = x;
	tmp->rect.y = y;

	if(tk->curTab->hasButtons == 0)
		tmp->next = 0;
	else
		tmp->next = tk->curTab->btnHead->next;
	tk->curTab->btnHead = tmp;
	tk->curTab->curBtn = tmp;

	tk->curTab->hasButtons = 1;
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

	initialTab->btnHead = 0;
	initialTab->curBtn = 0;

	return tmp;
}

int r_tk_draw(struct r_tk *tk)
{
	// draw tabs
	struct r_tk_tab *tmp;
	struct r_tk_btn *tmpBtn;
	int i = 0;

	tmp = tk->tabHead;
	while(1)
	{
		tmp->rect.x = i;
		i += tmp->rect.w + 10; // TODO: screen size scale
		if(tmp == tk->curTab)
		{
			SDL_SetTextureColorMod(tmp->text, 255, 0, 0);
			if(tmp->hasButtons == 1)
			{
				tmpBtn = tmp->btnHead;
				while(tmpBtn != 0)
				{
					// buttons
					if(tmp->btnHead == NULL || tmpBtn == NULL)
						break;

					if(tmpBtn == tk->curTab->curBtn)
						SDL_SetTextureColorMod(tmpBtn->text, 255, 0, 0);
					else
						SDL_SetTextureColorMod(tmpBtn->text, 255, 255, 255);

					SDL_RenderCopy(tk->renderer, tmpBtn->text, NULL, &tmpBtn->rect);
					if(tmpBtn->next == NULL)
						break;
					tmpBtn = tmpBtn->next;
				}
			}
		}
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
	new_btn(toolkit, "dupa", 20, 20);
	new_btn(toolkit, "dupa 2", 20, 50);
	new_btn(toolkit, "kurwaa", 20, 80);
	new_btn(toolkit, "dupa", 20, 110);
	new_tab(toolkit, "3");
	new_btn(toolkit, "3 btn", 30, 30);
	new_tab(toolkit, "four");
	new_btn(toolkit, "testinggg", 20, 30);

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
							break;
						case SDLK_DOWN:
							r_tk_next_btn(toolkit);
							break;
					}
					break;
			}
		}
	}

	//r_quit();
}
