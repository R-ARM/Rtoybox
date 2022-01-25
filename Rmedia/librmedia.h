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

	pthread_t _main_loop_thread;
};

void _run_gst_main_loop(void *rm)
{
	struct r_media *tmp = (struct r_media *)rm;
	g_main_loop_run(tmp->main_loop);
}

void rm_quit(struct r_media *tmp)
{
	g_main_loop_unref(tmp->main_loop);
	g_object_unref(tmp->bus);
	gst_element_set_state(tmp->playbin, GST_STATE_NULL);
	gst_object_unref(tmp->playbin);
}

static gboolean handle_bus(GstBus *bus, GstMessage *msg, gpointer data)
{
	GError *err;
	gchar *debug_info;
	switch(GST_MESSAGE_TYPE(msg))
	{
		case GST_MESSAGE_ERROR:
			gst_message_parse_error (msg, &err, &debug_info);
			log_err("Caught gst error: %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
			log_err("More info:: %s\n", debug_info ? debug_info : "none");
			g_clear_error (&err);
			g_free (debug_info);
			break;
	}
	return TRUE;
}

float get_sec_pos(struct r_media *tmp)
{
	gint64 pos;
	gst_element_query_position(tmp->playbin, GST_FORMAT_TIME, &pos);
	return (float)(pos / 1000000000); // convert into sec
}

void seek_sec(struct r_media *tmp, float interval)
{
	float pos = get_sec_pos(tmp);
	if(pos+interval < 0)
		gst_element_seek_simple(tmp->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0 * GST_SECOND);
	else
		gst_element_seek_simple(tmp->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (pos+interval) * GST_SECOND);
}

void force_new_track(struct r_media *tmp)
{
	gst_element_seek_simple(tmp->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 99999999 * GST_SECOND);
}

struct r_media * new_rm()
{
	struct r_media *tmp = malloc(sizeof(struct r_media));
	gst_init(0, 0);
	
	tmp->playbin = gst_element_factory_make("playbin", "playbin");
	if(!tmp->playbin)
	{
		log_err("Failed to initialize playbin element\n");
		exit(1);
	}
	
	g_object_get(tmp->playbin, "flags", &tmp->flags, NULL);
	tmp->flags = (1 << 1); // audio only for now
	g_object_set(tmp->playbin, "flags", tmp->flags, NULL);

	tmp->bus = gst_element_get_bus(tmp->playbin);
	gst_bus_add_watch(tmp->bus, (GstBusFunc)handle_bus, tmp);
	//g_signal_connect(data.playbin, "about-to-finish", TODO
	tmp->main_loop = g_main_loop_new(NULL, FALSE);

	pthread_create(&tmp->_main_loop_thread, NULL, _run_gst_main_loop, tmp);

	return tmp;
}
