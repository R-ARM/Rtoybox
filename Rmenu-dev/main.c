#include "../libragnarok.h"
#include <tgmath.h>

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

	struct r_tk_btn *prev;
	struct r_tk_btn *next;
};

struct r_tk_tab
{
	int id;

	int offsetY;
	int wantOffsetY;
	int offsetX;
	int wantOffsetX;

	char name[255];
	SDL_Rect rect;
	SDL_Texture *text;

	int isList;
	int hasButtons;
	struct r_tk_btn *curBtn;
	struct r_tk_btn *btnHead;
	struct r_tk_btn *btnTail;
	
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
	int previngTabs;
	int nextingTabs;

	struct r_tk_tab *oldTab;
	struct r_tk_tab *curTab;

	struct r_tk_tab *tabHead;
	struct r_tk_tab *tabTail;
};

// yes, those ARE internally reversed
void r_tk_next_tab(struct r_tk *tk)
{
	tk->curTab->offsetX = 0;
	tk->curTab->wantOffsetX = -480; // TODO: screen scaling
	
	tk->oldTab = tk->curTab;
	tk->curTab = tk->curTab->prev;
	
	tk->curTab->offsetX = 480;
	tk->curTab->wantOffsetX = 0;
}

void r_tk_prev_tab(struct r_tk *tk)
{
	tk->curTab->offsetX = 0;
	tk->curTab->wantOffsetX = 480;

	tk->oldTab = tk->curTab;	
	tk->curTab = tk->curTab->next;
	
	tk->curTab->offsetX = -480;
	tk->curTab->wantOffsetX = 0;
}

void r_tk_next_btn(struct r_tk *tk)
{
	if(tk->curTab->hasButtons == 1 && tk->curTab->curBtn->prev)
		tk->curTab->curBtn = tk->curTab->curBtn->prev;
}

void r_tk_prev_btn(struct r_tk *tk)
{
	if(tk->curTab->hasButtons == 1 && tk->curTab->curBtn->next)
		tk->curTab->curBtn = tk->curTab->curBtn->next;
}

void new_tab(struct r_tk *tk, char *name)
{
	struct r_tk_tab *tmp;

	tmp = malloc(sizeof(struct r_tk_tab));
	
	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strcpy(tmp->name, name);


	tmp->id = tk->tabHead->id + 1;
	tmp->hasButtons = 0;

	tmp->offsetY = 0;
	tmp->wantOffsetY = 0;
	tmp->offsetX = 0;
	tmp->wantOffsetX = 0;

	tmp->prev = tk->tabTail;
	tmp->next = tk->tabHead;

	tk->tabHead->prev = tmp;
	tk->tabHead = tmp;

	tk->tabHead->prev = tk->tabTail;
	tk->tabTail->next = tk->tabHead;
}

void new_btn(struct r_tk *tk, struct r_tk_tab *tab, char *name, int x, int y)
{
	struct r_tk_btn *tmp;

	tmp = malloc(sizeof(struct r_tk_btn));

	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strcpy(tmp->name, name);

	tmp->rect.x = x;
	tmp->rect.y = y;


	if(tab->btnHead == NULL || tab->hasButtons == 0)
	{
		tab->curBtn = tmp;
		tab->btnHead = tmp;
		tab->btnTail = tmp;
	}
	else
	{
		tmp->prev = tab->btnTail;
		tab->btnTail->next = tmp;
	}

	tab->btnTail = tmp;

	tab->btnHead->prev = 0;
	tab->btnTail->next = 0;

	tab->hasButtons = 1;
}

void new_btn_list_batch(struct r_tk *tk, struct r_tk_tab *tab, int num, ...)
{
	va_list valist;
	va_start(valist, num);

	struct r_tk_btn *tmp;
	char *name;

	for(int i = 0; i < num; i++)
	{
		name = va_arg(valist, char*);
		new_btn(tk, tab, name, 0, 0);
	}
	va_end(valist);
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
	tmp->oldTab = NULL;

	tmp->previngTabs = 0;
	tmp->nextingTabs = 0;

	initialTab->btnHead = 0;
	initialTab->curBtn = 0;
	initialTab->btnTail = 0;
	initialTab->hasButtons = 0;

	return tmp;
}

void draw_tab(struct r_tk *tk, struct r_tk_tab *tab)
{
	struct r_tk_btn *tmpBtn = tab->btnHead;
	int i = 0;

	while(tmpBtn != 0)
	{
		if(tmpBtn == tab->curBtn)
			SDL_SetTextureColorMod(tmpBtn->text, 255, 0, 0);
		else
			SDL_SetTextureColorMod(tmpBtn->text, 255, 255, 255);
		
		if(tab->isList == 1)
		{
			tmpBtn->rect.x = 0 + tab->offsetX;
			tmpBtn->rect.y = 25 * i - fmax(tab->offsetY, 0);

			SDL_RenderCopy(tk->renderer, tmpBtn->text, NULL, &tmpBtn->rect);

			tmpBtn->rect.x = 0;
			tmpBtn->rect.y = 25 * i;
		}
		else
		{
			tmpBtn->rect.x += tab->offsetX;
			tmpBtn->rect.y -= tab->offsetY;

			SDL_RenderCopy(tk->renderer, tmpBtn->text, NULL, &tmpBtn->rect);

			tmpBtn->rect.x -= tab->offsetX;
			tmpBtn->rect.y += tab->offsetY;
		}

		tmpBtn = tmpBtn->next;
		i++;
	}
}

int r_tk_draw(struct r_tk *tk)
{
	// draw tabs
	struct r_tk_tab *tmp;
	struct r_tk_btn *tmpBtn;

	SDL_Rect prevViewport;
	SDL_RenderGetViewport(tk->renderer, &prevViewport);

	int i = 0;
	tmp = tk->tabTail;
	do
	{
		tmp->rect.x = i;
		i += tmp->rect.w + 10; // TODO: screen size scale
		if(tmp == tk->curTab)
			SDL_SetTextureColorMod(tmp->text, 255, 0, 0);
		else
			SDL_SetTextureColorMod(tmp->text, 255, 255, 255);
		SDL_RenderCopy(tk->renderer, tmp->text, NULL, &tmp->rect);

		tmp = tmp->prev;
	} while(tmp != tk->tabTail);

	// line separating tabs and other widgets
	SDL_SetRenderDrawColor(tk->renderer, 255, 255, 255, 255);
	SDL_RenderDrawLine(tk->renderer, 0, 25, 480, 25);
	SDL_SetRenderDrawColor(tk->renderer, 0, 0, 0, 255);

	SDL_Rect area;
	area.x = 0;
	area.y = 25;
	area.w = 480; // TODO: screen size scale
	area.h = 320 - 25;

	SDL_RenderSetViewport(tk->renderer, &area);
	// draw buttons
	if(tk->curTab->wantOffsetX != tk->curTab->offsetX)
		tk->curTab->offsetX += (tk->curTab->wantOffsetX - tk->curTab->offsetX)/2;

	if(tk->curTab->hasButtons == 1)
	{
		tk->curTab->wantOffsetY = tk->curTab->curBtn->rect.y - 25; // TODO: scale
		if(tk->curTab->wantOffsetY != tk->curTab->offsetY)
			tk->curTab->offsetY += (tk->curTab->wantOffsetY - tk->curTab->offsetY)/2;
		draw_tab(tk, tk->curTab);
	}
	
	if(tk->oldTab != NULL)
	{
		if(tk->oldTab->wantOffsetX != tk->oldTab->offsetX)
			tk->oldTab->offsetX += (tk->oldTab->wantOffsetX - tk->oldTab->offsetX)/2;
		if(tk->oldTab->hasButtons == 1)
			draw_tab(tk, tk->oldTab);
		if(tk->oldTab->wantOffsetX - tk->oldTab->offsetX)
			tk->oldTab = NULL;
	}
	SDL_RenderSetViewport(tk->renderer, &prevViewport);
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

	new_btn(toolkit, toolkit->tabHead, "ss", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "dupa", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "tetwerg", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "qwertyoip", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "qr5y56p", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "asdjoeoip", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "placeholder", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "hokus pokus", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "twoja stara", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "to", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "twój stary", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "grwegwrg", 20, 0);
	new_btn(toolkit, toolkit->tabHead, "ertertg", 20, 0);
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "3");
	new_btn_list_batch(toolkit, toolkit->tabHead, 7, "3 btn", "buton", "i", "ran", "out", "of", "names");
	toolkit->tabHead->isList = 1;

	new_tab(toolkit, "four");

	new_btn(toolkit, toolkit->tabHead, "testinggg", 20, 30);
	new_btn(toolkit, toolkit->tabHead, "lower button 1", 20, 150);
	new_btn(toolkit, toolkit->tabHead, "right.", 300, 230);
	new_btn(toolkit, toolkit->tabHead, "lower button 2", 20, 200);
	new_btn(toolkit, toolkit->tabHead, "offscreen", 20, 330);
	toolkit->tabHead->isList = 0;

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
							r_tk_prev_btn(toolkit);
							break;
						case SDLK_UP:
							r_tk_next_btn(toolkit);
							break;
					}
					break;
			}
		}
	}

	//r_quit();
}
