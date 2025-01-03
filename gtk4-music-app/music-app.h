#ifndef MUSIC_APP_H
#define MUSIC_APP_H

#include <adwaita.h>
#include <gst/gst.h>
#include "music-config.h"
#include "mpris.h"

typedef struct _MusicPlayerApp {
    GtkWidget *window;
    GtkWidget *play_button;
    GtkWidget *stop_button;
    GtkWidget *volume_scale;
    GtkWidget *song_list;
    GtkWidget *progress_bar;
    GtkWidget *time_label;
    GstElement *pipeline;
    GstElement *source;
    GstElement *sink;
    GstElement *volume;
    GHashTable *music_library;
    char *current_file;
    gboolean is_playing;
    guint progress_id;
    guint total_files;
    guint processed_files;
    MusicConfig *config;
    MPRISData *mpris;
    GtkWidget *prev_button;
    GtkWidget *next_button;
} MusicPlayerApp;

#endif 