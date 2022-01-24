#include <gst/gst.h>

struct r_media
{
	GstBus *bus;
	GstStateChangeReturn ret;
	GIOChannel *io_stdin;

	gint flags;

	gint n_video;
	gint n_audio;
	gint n_text;

	gint c_video;
	gint c_audio;
	gint c_text;
	GstElement *playbin;
	GMainLoop *main_loop;
};

struct r_media * new_rmedia()
{
	struct r_media *tmp = malloc(sizeof(r_media));
	gst_init(0, 0);
	
	tmp->playbin = gst_element_factory_make("playbin", "playbin");
	if(!tmp->playbin)
	{
		log_error("Failed to initialize playbin element\n");
		exit(1);
	}
	
	g_object_get(tmp->playbin, "flags", &tmp->flags, NULL);
	tmp->flags = (1 << 1); // audio only for now
	g_object_set(tmp->playbin, "flags", tmp->flags, NULL);

	tmp->bus = gst_element_get_bus(tmp->playbin);
	//gst_bus_add_watch TODO
	//g_signal_connect(data.playbin, "about-to-finish", TODO
	tmp->main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(tmp->main_loop);

	// clean up
	g_main_loop_unref(tmp->main_loop);
	g_object_unref(tmp->bus);
	gst_element_set_state(tmp->playbin, GST_STATE_NULL);
	gst_object_unref(tmp->playbin);
}
