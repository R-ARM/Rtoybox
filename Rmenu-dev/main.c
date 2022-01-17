#include "../libragnarok.h"
#include "../librtoolkit.h"
#include <tgmath.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_tk *toolkit;

int main(void)
{
	r_init(&renderer, &window, &font, 0xff);
	toolkit = new_r_tk(&window, &renderer, &font, "Test");
	new_btn(toolkit, toolkit->tabHead, "this tab", 0, 0);
	new_btn(toolkit, toolkit->tabHead, "needs some", 30, 30);
	new_btn(toolkit, toolkit->tabHead, "buttons", 60, 60);
	toolkit->tabHead->scrolling = 0;

	new_cotab(toolkit, toolkit->tabHead, 300);
	new_btn_list_batch(toolkit, toolkit->tabHead->coTab, 4, "this", "is", "a", "test");
	toolkit->tabHead->coTab->isList = 1;

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
					case SDLK_a:
					case SDLK_s:
						r_tk_toggle_cotab(toolkit);
						break;
					case SDLK_z:
						r_tk_prev_btn(toolkit);
						break;
					case SDLK_x:
						r_tk_next_btn(toolkit);
						break;
					}
			}
		}
	}
}
