#include <tgmath.h>
#include <semaphore.h>

void get_text_and_rect(SDL_Renderer *renderer, char *text,
	TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect, int r, int g, int b)
{
	int text_width;
	int text_height;
	SDL_Surface *surface;
	SDL_Color textColor = {r, g, b, 0};

	surface = TTF_RenderUTF8_Blended(font, text, textColor);
	*texture = SDL_CreateTextureFromSurface(renderer, surface);
	text_width = surface->w;
	text_height = surface->h;
	SDL_FreeSurface(surface);
	rect->x = 0;
	rect->y = 0;
	rect->w = text_width;
	rect->h = text_height;
}

#define BTN_TYPE_CLICK		0
#define BTN_TYPE_TOGGLE		1
#define BTN_TYPE_ONEOF		2
#define BTN_TYPE_ONEOF_CHILD	3

#define BTN_STATEPOS_LEFT	0
#define BTN_STATEPOS_RIGHT	1
#define BTN_STATEPOS_CUSTOM	2

union buttonState {
	int integer;
	void *pointer;
};

struct r_tk_btn
{
	int id;
	union buttonState state;
	int type;

	int statePositioning;
	int forceStateX;

	char name[255];
	void *progData;

	SDL_Rect rect;
	SDL_Texture *text;

	struct r_tk_tab *coTab;

	struct r_tk_btn *prev;
	struct r_tk_btn *next;
};

struct r_tk_tab
{
	int id;
	void *progData;

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

	struct r_tk_tab *curCoTab;
	int isCoTab;
	int coTabAct;

	struct r_tk_tab *next;
	struct r_tk_tab *prev;
};

struct r_tk
{
	SDL_Window **window;
	SDL_Renderer **renderer;
	TTF_Font **font;

	int inputTabSwitching;
	void (*btn_cb)(struct r_tk_btn*);

	int width;
	int height;

	int tabOffsetX;
	int tabWantOffsetX;

	int lastBtnId;
	int fontsize;

	sem_t draw_done_sem;
	sem_t draw_start_sem;

	struct r_tk_tab *oldTab;
	struct r_tk_tab *curTab;

	struct r_tk_tab *tabHead;
	struct r_tk_tab *tabTail;
};
struct r_tk *_r_glob_toolkit;

// yes, those ARE internally reversed
void r_tk_next_tab(struct r_tk *tk)
{
	if(tk->curTab != tk->curTab->prev)
	{
		tk->curTab->offsetX = 0;
		tk->curTab->wantOffsetX = -1 * tk->width; // TODO: screen scaling

		tk->oldTab = tk->curTab;
		tk->curTab = tk->curTab->prev;

		tk->curTab->offsetX = tk->width;
		tk->curTab->wantOffsetX = 0;
	}
	sem_post(&tk->draw_start_sem);
}

void r_tk_prev_tab(struct r_tk *tk)
{
	if(tk->curTab != tk->curTab->next)
	{
		tk->curTab->offsetX = 0;
		tk->curTab->wantOffsetX = tk->width;

		tk->oldTab = tk->curTab;
		tk->curTab = tk->curTab->next;

		tk->curTab->offsetX = -1 * tk->width;
		tk->curTab->wantOffsetX = 0;
	}
	sem_post(&tk->draw_start_sem);
}

void r_tk_toggle_cotab(struct r_tk *tk)
{
	if(tk->curTab->curCoTab != 0)
		tk->curTab->coTabAct = !tk->curTab->coTabAct;
	sem_post(&tk->draw_start_sem);
}

void r_tk_next_btn(struct r_tk *tk)
{
	if(tk->curTab->coTabAct == 1)
	{
		if(tk->curTab->curCoTab->hasButtons == 1 && tk->curTab->curCoTab->curBtn->prev)
			tk->curTab->curCoTab->curBtn = tk->curTab->curCoTab->curBtn->prev;
	}
	else
	{
		if(tk->curTab->hasButtons == 1 && tk->curTab->curBtn->prev)
			tk->curTab->curBtn = tk->curTab->curBtn->prev;
	}
	sem_post(&tk->draw_start_sem);
}

void r_tk_prev_btn(struct r_tk *tk)
{
	if(tk->curTab->coTabAct == 1)
	{
		if(tk->curTab->curCoTab->hasButtons == 1 && tk->curTab->curCoTab->curBtn->next)
			tk->curTab->curCoTab->curBtn = tk->curTab->curCoTab->curBtn->next;
	}
	else
	{
		if(tk->curTab->hasButtons == 1 && tk->curTab->curBtn->next)
			tk->curTab->curBtn = tk->curTab->curBtn->next;
	}
	sem_post(&tk->draw_start_sem);
}

struct r_tk_tab * _new_tab(struct r_tk *tk, char *name)
{
	struct r_tk_tab *tmp;

	tmp = calloc(1, sizeof(struct r_tk_tab));

	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strncpy(tmp->name, name, 254);

	tmp->id = tk->tabHead->id + 1;

	tmp->scrolling = 1;

	sem_post(&tk->draw_start_sem);
	return tmp;
}

struct r_tk_tab* new_tab(struct r_tk *tk, char *name)
{
	struct r_tk_tab *tmp;
	tmp = _new_tab(tk, name);
	tmp->prev = tk->tabTail;
	tmp->next = tk->tabHead;
	tmp->curCoTab = 0;
	tmp->coTabAct = 0;

	tk->tabHead->prev = tmp;
	tk->tabHead = tmp;

	tk->tabHead->prev = tk->tabTail;
	tk->tabTail->next = tk->tabHead;

	sem_post(&tk->draw_start_sem);
	return tmp;
}

struct r_tk_btn * new_btn(struct r_tk *tk, struct r_tk_tab *tab, char *name, int x, int y)
{
	struct r_tk_btn *tmp;

	tmp = calloc(1, sizeof(struct r_tk_btn));

	get_text_and_rect(tk->renderer, name, *tk->font, &tmp->text, &tmp->rect, 255, 255, 255);
	strncpy(tmp->name, name, 254);

	tmp->id = tk->lastBtnId++;
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

	tab->hasButtons = 1;

	sem_post(&tk->draw_start_sem);
	return tmp;
}

struct r_tk_btn * new_toggle(struct r_tk *tk, struct r_tk_tab *tab, char *name, int x, int y, int initState, int stateAlign, int statePosition)
{
	struct r_tk_btn *tmp;
	tmp = new_btn(tk, tab, name, x, y);

	tmp->type = BTN_TYPE_TOGGLE;
	tmp->state.integer = initState;
	tmp->statePositioning = stateAlign;
	tmp->forceStateX = statePosition;

	return tmp;
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

struct r_tk_tab * new_cotab(struct r_tk *tk, struct r_tk_tab *tab, struct r_tk_btn *base)
{
	base->coTab = _new_tab(tk, base->name);
	tab->curCoTab = base->coTab;
	base->coTab->curCoTab = tab;
	base->coTab->isCoTab = 1;
	base->coTab->scrolling = 1;
	base->coTab->isList = 1;
	return base->coTab;
}

struct r_tk_btn * new_oneof_opt(struct r_tk *tk, struct r_tk_btn *parent, char *name, void *progdata)
{
	struct r_tk_btn *tmp = new_btn(tk, parent->coTab, name, 0, 0);
	tmp->type = BTN_TYPE_ONEOF_CHILD;
	tmp->progData = (progdata == NULL) ? 0 : progdata;

	int newOffsetX = tk->width - (tmp->rect.w + 5);
	if(newOffsetX < parent->coTab->offsetX)
		parent->coTab->offsetX = newOffsetX;

	return tmp;
}

struct r_tk_btn * new_oneof(struct r_tk *tk, struct r_tk_tab *tab, char *name, int x, int y, int num, ...)
{
	va_list valist;
	va_start(valist, num);

	struct r_tk_btn *oneof = new_btn(tk, tab, name, x, y);
	oneof->type = BTN_TYPE_ONEOF;

	struct r_tk_tab *cotab = new_cotab(tk, tab, oneof);
	cotab->offsetX = tk->width; // offscreen, until a button is added

	char *choice;
	struct r_tk_btn *tmp;

	oneof->state.pointer = 0;

	for(int i = 0; i < num; i++)
	{
		choice = va_arg(valist, char*);
		tmp = new_oneof_opt(tk, oneof, choice, 0);

		if(i == 0)
			oneof->state.pointer = tmp;
	}
	va_end(valist);

	tab->curCoTab = 0;

	sem_post(&tk->draw_start_sem);
	return oneof;
}

struct r_tk * new_r_tk(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font, char* initTabName, void (*cb)(struct r_tk_btn *btn))
{
	struct r_tk *tmp;

	tmp = calloc(1, sizeof(struct r_tk));
	tmp->window = window;
	tmp->renderer = *renderer;
	tmp->font = font;
	tmp->btn_cb = cb;
	tmp->inputTabSwitching = 1;

	struct r_tk_tab *initialTab;
	initialTab = calloc(1, sizeof(struct r_tk_tab));
	initialTab->next = initialTab;
	initialTab->prev = initialTab;
	initialTab->scrolling = 1;
	strncpy(initialTab->name, initTabName, 254);

	get_text_and_rect(*renderer, initTabName, *font, &initialTab->text, &initialTab->rect, 255, 255, 255);

	tmp->tabHead = initialTab;
	tmp->tabTail = initialTab;
	tmp->curTab = initialTab;
	tmp->oldTab = NULL;

	tmp->fontsize = _fontsize; // XXX: global

	SDL_GetWindowSize(*window, &tmp->width, &tmp->height);

	sem_init(&tmp->draw_done_sem, 0, 1);
	sem_init(&tmp->draw_start_sem, 0, 1);
	sem_post(&tmp->draw_start_sem);

	_r_glob_toolkit = tmp;
	return tmp;
}

void destroy_btn(struct r_tk *tk, struct r_tk_tab *tab, struct r_tk_btn *button)
{
	if(button->coTab)
	{
		while(button->coTab->btnHead)
			destroy_btn(tk, button->coTab, button->coTab->btnHead);

		SDL_DestroyTexture(button->coTab->text);
		free(button->coTab);
	}

	// if we're head or tail handle that too
	if(tab->btnHead == button && tab->btnTail == button)
	{
		tab->btnHead = 0;
		tab->btnTail = 0;
		tab->hasButtons = 0;
	} else if(tab->btnHead == button)
		tab->btnHead = tab->btnHead->next;
	else if(tab->btnTail == button)
		tab->btnTail = tab->btnTail->prev;

	if(button->prev && button->next)
	{
		button->prev->next = button->next;
		button->next->prev = button->prev;
	} else if(button->next)
		button->next->prev = 0;
	else if(button->prev)
		button->prev->next = 0;

	if(tab->curBtn == button)
	{
		if(tab->curBtn->prev == button)
			tab->curBtn = 0;
		else
			if(tab->curBtn->prev)
				tab->curBtn = tab->curBtn->prev;
			else if(tab->curBtn->next)
				tab->curBtn = tab->curBtn->next;
			else
				tab->curBtn = 0;
	}

	SDL_DestroyTexture(button->text);
	free(button);
}

void destroy_tab(struct r_tk *tk, struct r_tk_tab *tab)
{
	if(tk->tabHead == tab && tk->tabTail == tab)
	{
		// TODO?
		log_warn("Can't remove last tab\n");
		return;
	}
	else if(tk->tabHead == tab)
		tk->tabHead = tk->tabHead->next;
	else if(tk->tabTail == tab)
		tk->tabTail = tk->tabTail->prev;

	tk->tabHead->prev = tk->tabTail;
	tk->tabTail->next = tk->tabHead;

	if(tab->prev)
		tab->prev->next = tab->next;
	if(tab->next)
		tab->next->prev = tab->prev;

	if(tk->curTab == tab)
	{
		if(tk->curTab->prev)
			tk->curTab = tk->curTab->prev;
		else if(tk->curTab->next)
			tk->curTab = tk->curTab->next;
		else
			log_err("No tab found to switch to after removal of current one\n");
	}

	while(tab->btnHead)
		destroy_btn(tk, tab, tab->btnHead);

	SDL_DestroyTexture(tab->text);
	free(tab);
}

void destroy_toolkit(struct r_tk *tk)
{
	while(tk->tabHead != tk->tabTail)
	{
		if(tk->tabHead->progData)
			free(tk->tabHead->progData);
		destroy_tab(tk, tk->tabHead);
	}
	// last tab
	while(tk->tabHead->btnHead)
		destroy_btn(tk, tk->tabHead, tk->tabHead->btnHead);

	SDL_DestroyRenderer(tk->renderer);
	SDL_DestroyWindow(tk->window);
	free(tk);

	SDL_Quit();
}

void _draw_tab(struct r_tk *tk, struct r_tk_tab *tab)
{
	struct r_tk_btn *tmpBtn = tab->btnHead;
	int i = 0;

	while(tmpBtn != 0)
	{
		if(tmpBtn == tab->curBtn)
			if (tk->curTab->coTabAct == (tab == tk->curTab->curCoTab))
				SDL_SetTextureColorMod(tmpBtn->text, 255, 0, 0);
			else
				SDL_SetTextureColorMod(tmpBtn->text, 120, 0, 0);
		else
			SDL_SetTextureColorMod(tmpBtn->text, 255, 255, 255);

		if(tab->isList == 1)
		{
			tmpBtn->rect.x = 0 + tab->offsetX;
			tmpBtn->rect.y = (tk->fontsize + 1) * i - fmax(tab->offsetY, 0);

			draw_btn(tk, tmpBtn);

			tmpBtn->rect.x = 0;
			tmpBtn->rect.y = (tk->fontsize + 1) * i;
		}
		else
		{
			tmpBtn->rect.x += tab->offsetX;
			tmpBtn->rect.y -= tab->offsetY;

			draw_btn(tk, tmpBtn);

			tmpBtn->rect.x -= tab->offsetX;
			tmpBtn->rect.y += tab->offsetY;
		}

		tmpBtn = tmpBtn->next;
		i++;
	}
}

void draw_btn(struct r_tk *tk, struct r_tk_btn *btn)
{
	int margin = 2;

	if(btn->rect.y + btn->rect.h < 0)
		return;

	if(btn->type == BTN_TYPE_TOGGLE)
	{
		struct SDL_Rect toggleRect;

		toggleRect.w = btn->rect.h - margin*2;
		toggleRect.h = btn->rect.h - margin*2;

		switch(btn->statePositioning)
		{
			case BTN_STATEPOS_LEFT:
				// make a square with padding of 1px
				toggleRect.x = btn->rect.x + margin;
				toggleRect.y = btn->rect.y + margin;
				btn->rect.x += tk->fontsize + margin*2;
				break;
			case BTN_STATEPOS_RIGHT:
				// ditto
				toggleRect.x = btn->rect.x + margin + btn->rect.w;
				toggleRect.y = btn->rect.y + margin;
				break;
			case BTN_STATEPOS_CUSTOM:
				toggleRect.x = btn->forceStateX;
				toggleRect.y = btn->rect.y + margin;
				break;
		}

		SDL_SetRenderDrawColor(tk->renderer, 255, 255, 255, 255); // TODO: coloring

		if(btn->state.integer == 1)
			SDL_RenderFillRect(tk->renderer, &toggleRect);
		else
			SDL_RenderDrawRect(tk->renderer, &toggleRect);
	}


	SDL_RenderCopy(tk->renderer, btn->text, NULL, &btn->rect);

	if(btn->type == BTN_TYPE_TOGGLE && btn->statePositioning == BTN_STATEPOS_LEFT)
		btn->rect.x -= tk->fontsize + margin*2;
}

void draw_tab(struct r_tk *tk, struct r_tk_tab *tab)
{
	_draw_tab(tk, tab);

	if(tk->curTab->hasButtons && tk->curTab->curBtn->type == BTN_TYPE_ONEOF)
	{
		tab->curCoTab = tk->curTab->curBtn->coTab;
		tab->curCoTab->offsetX += tab->offsetX;
		_draw_tab(tk, tab->curCoTab);
		tab->curCoTab->offsetX -= tab->offsetX;
	}
	else
		tab->curCoTab = 0;
}

void r_tk_action(struct r_tk *tk)
{
	struct r_tk_btn *tmp = NULL;
	if(tk->btn_cb == NULL) return;
	if(tk->curTab->curCoTab != 0 && tk->curTab->coTabAct == 1)
		tmp = tk->curTab->curCoTab->curBtn;
	else
		tmp = tk->curTab->curBtn;

	if(tmp->type == BTN_TYPE_TOGGLE)
	{
		tmp->state.integer = !tmp->state.integer;
		sem_post(&tk->draw_start_sem);
	}

	if(tmp->type == BTN_TYPE_ONEOF)
	{
		r_tk_toggle_cotab(tk);
		return;
	}

	if(tmp->type == BTN_TYPE_ONEOF_CHILD)
		r_tk_toggle_cotab(tk);

	if(tmp != NULL && tmp != 0)
		tk->btn_cb(tmp);
}

int _r_tk_input_handler(int type, int code, int value)
{
	if(value == 0) return 0;
	struct r_tk *toolkit = _r_glob_toolkit; // :( ugly

	sem_wait(&toolkit->draw_done_sem);
	// semaphores maybe???
	switch(code)
	{
		case BTN_DPAD_UP:
			r_tk_next_btn(toolkit);
			break;
		case BTN_DPAD_DOWN:
			r_tk_prev_btn(toolkit);
			break;
		case BTN_DPAD_LEFT:
		case BTN_DPAD_RIGHT:
			r_tk_toggle_cotab(toolkit);
			break;
		case BTN_EAST:
		case BTN_SOUTH:
			r_tk_action(toolkit);
			break;
		case BTN_TR:
			if(toolkit->inputTabSwitching == 1)
				r_tk_next_tab(toolkit);
			break;
		case BTN_TL:
			if(toolkit->inputTabSwitching == 1)
				r_tk_prev_tab(toolkit);
			break;
	}

	sem_post(&toolkit->draw_start_sem);
	return 0;

	//out:
	//return _r_tk_prog_input(type, code, value); // maybe in future?
}

int r_tk_draw(struct r_tk *tk)
{
	// draw tabs
	struct r_tk_tab *tmp;
	struct r_tk_btn *tmpBtn;


	//log_debug("Need to redraw? %d\n", tk->reDraw);
	sem_wait(&tk->draw_start_sem);

	SDL_Rect prevViewport;
	SDL_RenderGetViewport(tk->renderer, &prevViewport);

	SDL_SetRenderDrawColor(tk->renderer, 0, 0, 0, 255);
	SDL_RenderClear(tk->renderer);
	int i = 0;
	if(tk->tabTail != tk->tabHead)
	{
		SDL_Rect tabs;
		tabs.x = 0;
		tabs.y = 0;
		tabs.w = tk->width;
		tabs.h = tk->tabHead->rect.h;

		SDL_RenderSetViewport(tk->renderer, &tabs);
		tmp = tk->tabTail;
		int shouldScroll = tk->tabHead->rect.x + tk->tabHead->rect.w > tk->width;
		int maxScroll = tk->tabHead->rect.x + tk->tabHead->rect.w;
		tk->tabWantOffsetX = tk->curTab->rect.x;
		if (shouldScroll)
		{
			if (fabs(tk->tabWantOffsetX - tk->tabOffsetX) < 3)
				tk->tabOffsetX = tk->tabWantOffsetX;
			if (shouldScroll && tk->tabWantOffsetX != tk->tabOffsetX)
				tk->tabOffsetX += (tk->tabWantOffsetX - tk->tabOffsetX)/2;
		}
		do
		{
			tmp->rect.x = i - tk->tabOffsetX;

			if(tmp == tk->curTab)
				SDL_SetTextureColorMod(tmp->text, 255, 0, 0);
			else
				SDL_SetTextureColorMod(tmp->text, 255, 255, 255);
			SDL_RenderCopy(tk->renderer, tmp->text, NULL, &tmp->rect);

			tmp->rect.x = i;

			i += tmp->rect.w + 10; // TODO: screen size scale
			tmp = tmp->prev;
		} while(tmp != tk->tabTail);

		SDL_Rect area;
		area.x = 0;
		area.y = tk->tabHead->rect.h + 1;
		area.w = tk->width;
		area.h = tk->height - (tk->tabHead->rect.h + 1);

		SDL_RenderSetViewport(tk->renderer, &area);

		// line separating tabs and other widgets
		SDL_SetRenderDrawColor(tk->renderer, 255, 255, 255, 255);
		SDL_RenderDrawLine(tk->renderer, 0, 0, tk->width, 0);
		SDL_SetRenderDrawColor(tk->renderer, 0, 0, 0, 255);
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
			tk->curTab->wantOffsetY = tk->curTab->curBtn->rect.y - tk->fontsize - 1;
			if(tk->curTab->wantOffsetY != tk->curTab->offsetY)
				if(fabs(tk->curTab->wantOffsetY - tk->curTab->offsetY) < 2)
					tk->curTab->offsetY = tk->curTab->wantOffsetY;
				else
					tk->curTab->offsetY += (tk->curTab->wantOffsetY - tk->curTab->offsetY)/2;
		}
		if(tk->curTab->curCoTab != 0 && tk->curTab->curCoTab->scrolling == 1)
		{
			tk->curTab->curCoTab->wantOffsetY = tk->curTab->curCoTab->curBtn->rect.y - tk->fontsize - 1;
			if(tk->curTab->curCoTab->wantOffsetY != tk->curTab->curCoTab->offsetY)
				if(fabs(tk->curTab->curCoTab->offsetY - tk->curTab->curCoTab->wantOffsetY) < 2)
					tk->curTab->curCoTab->offsetY = tk->curTab->curCoTab->wantOffsetY;
				else
					tk->curTab->curCoTab->offsetY += (tk->curTab->curCoTab->wantOffsetY - tk->curTab->curCoTab->offsetY)/2;
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

	if((tk->curTab->wantOffsetX != tk->curTab->offsetX) || (tk->curTab->wantOffsetY != tk->curTab->offsetY))
	{
		if(tk->curTab->curCoTab)
		{
			if((tk->curTab->curCoTab->wantOffsetY != tk->curTab->curCoTab->offsetY))
				sem_post(&tk->draw_start_sem);
		}
		else
		{
			sem_post(&tk->draw_start_sem);
		}
	}

	//log_debug("want x = %d, x = %d, want y = %d, y = %d\n", tk->curTab->wantOffsetX, tk->curTab->offsetX, tk->curTab->wantOffsetY, tk->curTab->offsetY);

	SDL_RenderSetViewport(tk->renderer, &prevViewport);
	SDL_RenderPresent(tk->renderer);

	sem_post(&tk->draw_done_sem);
}
