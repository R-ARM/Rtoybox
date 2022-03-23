#include "../libragnarok.h"
#include "../librtoolkit.h"
#include "librmedia.h"
#include <tgmath.h>
#include <dirent.h>

SDL_Renderer *renderer;
SDL_Window *window;
TTF_Font *font;

struct r_media *md;
struct r_tk *toolkit;
int numFiles = 0;
char *nextPath = NULL;

void buttonStateCallback(struct r_tk_btn *btn)
{
	log_debug("Forcing new file and forcing new track\n");
	nextPath = (char *)btn->progData;
	force_new_track(md);
}

char* getRandomFile(struct r_tk_tab *tab)
{
	struct r_tk_btn *button = tab->btnTail;
	int randId = rand() % numFiles;
	log_debug("Loading fileID %d\n", randId);
	while(button->id != randId) // TODO: timeout
		button = button->prev;
	log_debug("Random filename: %s\n", (char *)button->progData);
	return (char *)button->progData;
}

static gboolean load_new (GstElement * playbin, gpointer udata)
{
	log_debug("Loading new file\n");
	// TODO: not shuffle
	char newUri[256] = "";
	if(nextPath != NULL)
	{
		strncat(newUri, nextPath, 248);
		nextPath = NULL;
	}
	else
	{
		strncat(newUri, getRandomFile(toolkit->curTab), 248);
	}

	log_debug("Playing: %s\n", newUri);
	g_object_set(playbin, "uri", newUri, NULL);
	return TRUE;
}

int main(void)
{
	r_init(&renderer, &window, &font, 0xff, 24);
	toolkit = new_r_tk(&window, &renderer, &font, "Tracks", buttonStateCallback);
	md = new_rm();
	srand(time(0));
	char *nowPlaying;
	DIR *d;
	struct dirent *ent;
#ifdef ROS
	chdir("/music/");
#else
	chdir("./music");
#endif
	d = opendir("./");
	if(d)
	{
		char *path;
		char *name;
		int pathLen = 0;
		int nameLen = 0;
		while((ent = readdir(d)) != NULL)
		{
			if(ent->d_type == DT_REG)
			{
				log_debug("Adding file %s\n", ent->d_name);

				pathLen = strlen(realpath(ent->d_name, NULL)) + strlen("file://");
				path = malloc(pathLen);
				strcpy(path, "file://");
				strncat(path, realpath(ent->d_name, NULL), pathLen - strlen("file://"));
				
				nameLen = strcspn(ent->d_name, ".");
				name = malloc(nameLen);
				strncpy(name, ent->d_name, nameLen);


				new_btn(toolkit, toolkit->tabHead, name, 0, 0);
				toolkit->tabHead->btnTail->progData = path;
				
				free(name);
				numFiles++;
			}
		}
		free(d);
	}
	else
	{
		log_err("Failed opening directory\n");
		exit(1);
	}
	log_debug("Loaded %d files\n", numFiles);
	load_new(md->playbin, NULL);

	g_signal_connect(md->playbin, "about-to-finish", G_CALLBACK(load_new), NULL);
	int ret;
	ret = gst_element_set_state(md->playbin, GST_STATE_PLAYING);
	if(ret == GST_STATE_CHANGE_FAILURE)
	{
		log_err("Failed to set pipeline into playing state\n");
		return 1;
	}
	md->playing = 1;


	toolkit->tabHead->isList = 1;

	SDL_Event event;
	while (1)
	{
		SDL_RenderClear(renderer);
		r_tk_draw(toolkit);
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
					case SDLK_w: // trigger right
						force_new_track(md);
						break;
					case SDLK_q: // trigger left
						// prev track TODO
						break;
					case SDLK_a: // left
						seek_sec(md, -5);
						break;
					case SDLK_s: // right
						seek_sec(md, 5);
						break;
					case SDLK_z: // down
						r_tk_prev_btn(toolkit);
						break;
					case SDLK_x: // up
						r_tk_next_btn(toolkit);
						break;
					case SDLK_e: // A
						// load new track
						r_tk_action(toolkit);
						break;
					case SDLK_t: // B
						play_pause(md);
						break;
					}
			}
		}
	}
}
