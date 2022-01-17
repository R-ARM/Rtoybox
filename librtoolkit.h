#include <tgmath.h>

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
	int state;
	char name[255];
	SDL_Rect rect;
	SDL_Texture *text;

	void *ptr;

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
	int scrolling;

	char name[255];
	SDL_Rect rect;
	SDL_Texture *text;

	int isList;
	int hasButtons;
	struct r_tk_btn *curBtn;
	struct r_tk_btn *btnHead;
	struct r_tk_btn *btnTail;
	
	struct r_tk_tab *coTab;
	int coTabAct;

	struct r_tk_tab *next;
	struct r_tk_tab *prev;
};

struct r_tk
{
	SDL_Window **window;
	SDL_Renderer **renderer;
	TTF_Font **font;

	void (*btn_cb)(struct r_tk_btn*);

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
	if(tk->curTab != tk->curTab->prev)
	{
		tk->curTab->offsetX = 0;
		tk->curTab->wantOffsetX = -480; // TODO: screen scaling
	
		tk->oldTab = tk->curTab;
		tk->curTab = tk->curTab->prev;
	
		tk->curTab->offsetX = 480;
		tk->curTab->wantOffsetX = 0;
	}
}

void r_tk_prev_tab(struct r_tk *tk)
{
	if(tk->curTab != tk->curTab->next)
	{
		tk->curTab->offsetX = 0;
		tk->curTab->wantOffsetX = 480;

		tk->oldTab = tk->curTab;	
		tk->curTab = tk->curTab->next;
	
		tk->curTab->offsetX = -480;
		tk->curTab->wantOffsetX = 0;
	}
}

void r_tk_toggle_cotab(struct r_tk *tk)
{
	if(tk->curTab->coTab != 0)
		tk->curTab->coTabAct = !tk->curTab->coTabAct;
}

void r_tk_next_btn(struct r_tk *tk)
{
	if(tk->curTab->coTabAct == 1)
	{
		if(tk->curTab->coTab->hasButtons == 1 && tk->curTab->coTab->curBtn->prev)
			tk->curTab->coTab->curBtn = tk->curTab->coTab->curBtn->prev;
	}
	else
	{
		if(tk->curTab->hasButtons == 1 && tk->curTab->curBtn->prev)
			tk->curTab->curBtn = tk->curTab->curBtn->prev;
	}
}

void r_tk_prev_btn(struct r_tk *tk)
{
	if(tk->curTab->coTabAct == 1)
	{
		if(tk->curTab->coTab->hasButtons == 1 && tk->curTab->coTab->curBtn->next)
			tk->curTab->coTab->curBtn = tk->curTab->coTab->curBtn->next;
	}
	else
	{
		if(tk->curTab->hasButtons == 1 && tk->curTab->curBtn->next)
			tk->curTab->curBtn = tk->curTab->curBtn->next;
	}
}

struct r_tk_tab * _new_tab(struct r_tk *tk, char *name)
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
	tmp->scrolling = 1;

	return tmp;
}

void new_tab(struct r_tk *tk, char *name)
{
	struct r_tk_tab *tmp;
	tmp = _new_tab(tk, name);
	tmp->prev = tk->tabTail;
	tmp->next = tk->tabHead;
	tmp->coTab = 0;
	tmp->coTabAct = 0;

	tk->tabHead->prev = tmp;
	tk->tabHead = tmp;

	tk->tabHead->prev = tk->tabTail;
	tk->tabTail->next = tk->tabHead;
}

struct r_tk_tab * new_cotab(struct r_tk *tk, struct r_tk_tab *other, int offset)
{
	other->coTab = _new_tab(tk, other->name);

	other->coTab->offsetX = offset;
	other->coTab->coTab = other;
}

void new_btn(struct r_tk *tk, struct r_tk_tab *tab, char *name, int x, int y)
{
	struct r_tk_btn *tmp;

	tmp = malloc(sizeof(struct r_tk_btn));

	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strcpy(tmp->name, name);

	tmp->rect.x = x;
	tmp->rect.y = y;
	tmp->state = 0;

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

struct r_tk * new_r_tk(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font, char* initTabName, void (*cb)(struct r_tk_btn *btn))
{
	struct r_tk *tmp;

	tmp = malloc(sizeof(struct r_tk));
	tmp->window = window;
	tmp->renderer = *renderer;
	tmp->font = font;
	tmp->btn_cb = cb;

	struct r_tk_tab *initialTab;
	initialTab = malloc(sizeof(struct r_tk_tab));
	initialTab->id = 0;
	initialTab->next = initialTab;
	initialTab->prev = initialTab;
	initialTab->offsetX = 0;
	initialTab->offsetY = 0;
	initialTab->wantOffsetX = 0;
	initialTab->wantOffsetY = 0;
	initialTab->scrolling = 1;
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

void _draw_tab(struct r_tk *tk, struct r_tk_tab *tab)
{
	struct r_tk_btn *tmpBtn = tab->btnHead;
	int i = 0;

	while(tmpBtn != 0)
	{
		if(tmpBtn == tab->curBtn)
			if (tk->curTab->coTabAct == (tab == tk->curTab->coTab))
				SDL_SetTextureColorMod(tmpBtn->text, 255, 0, 0);
			else
				SDL_SetTextureColorMod(tmpBtn->text, 120, 0, 0);
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

void draw_tab(struct r_tk *tk, struct r_tk_tab *tab)
{
	_draw_tab(tk, tab);

	if(tab->coTab != 0)
	{
		tab->coTab->offsetX = tab->offsetX + 300;
		_draw_tab(tk, tab->coTab);
		tab->coTab->offsetX = tab->offsetX;
	}
}

void r_tk_action(struct r_tk *tk)
{
	if(tk->curTab->coTab != 0 && tk->curTab->coTabAct == 1)
		tk->btn_cb(tk->curTab->coTab->curBtn);
	else
		tk->btn_cb(tk->curTab->curBtn);
}

int r_tk_draw(struct r_tk *tk, int width)
{
	// draw tabs
	struct r_tk_tab *tmp;
	struct r_tk_btn *tmpBtn;

	SDL_Rect prevViewport;
	SDL_RenderGetViewport(tk->renderer, &prevViewport);

	int i = 0;
	if(tk->tabTail != tk->tabHead)
	{
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
		area.w = width; // TODO: screen size scale
		area.h = 320 - 25;

		SDL_RenderSetViewport(tk->renderer, &area);
	}
	// draw buttons
	if(tk->curTab->wantOffsetX != tk->curTab->offsetX)
		tk->curTab->offsetX += (tk->curTab->wantOffsetX - tk->curTab->offsetX)/4;
	if(fabs(tk->curTab->wantOffsetX - tk->curTab->offsetX) < 4)
		tk->curTab->offsetX = tk->curTab->wantOffsetX;

	if(tk->curTab->hasButtons == 1)
	{
		if(tk->curTab->scrolling == 1)
		{
			tk->curTab->wantOffsetY = tk->curTab->curBtn->rect.y - 25; // TODO: scale
			if(tk->curTab->wantOffsetY != tk->curTab->offsetY)
				tk->curTab->offsetY += (tk->curTab->wantOffsetY - tk->curTab->offsetY)/2;
		}
		if(tk->curTab->coTab && tk->curTab->coTab->scrolling == 1)
		{
			tk->curTab->coTab->wantOffsetY = tk->curTab->coTab->curBtn->rect.y - 25;
			if(tk->curTab->coTab->wantOffsetY != tk->curTab->coTab->offsetY)
				tk->curTab->coTab->offsetY += (tk->curTab->coTab->wantOffsetY - tk->curTab->coTab->offsetY)/2;
		}
		draw_tab(tk, tk->curTab);
	}
	
	if(tk->oldTab != NULL)
	{
		if(tk->oldTab->wantOffsetX != tk->oldTab->offsetX)
			tk->oldTab->offsetX += (tk->oldTab->wantOffsetX - tk->oldTab->offsetX)/4;
		if(fabs(tk->curTab->wantOffsetX - tk->curTab->offsetX) < 4)
			tk->oldTab->offsetX = tk->oldTab->wantOffsetX;
		if(tk->oldTab->hasButtons == 1)
			draw_tab(tk, tk->oldTab);
		if(tk->oldTab->wantOffsetX - tk->oldTab->offsetX)
			tk->oldTab = NULL;
	}
	SDL_RenderSetViewport(tk->renderer, &prevViewport);
}
