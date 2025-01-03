#include "music-app.h"
#include <adwaita.h>
#include <gst/gst.h>
#include <glib/gstdio.h>
#include "music-config.h"
#include "mpris.h"

static void on_folder_response(GtkNativeDialog *native, GAsyncResult *result, MusicPlayerApp *app);
static void scan_music_directory(MusicPlayerApp *app, const char *path);
static void count_audio_files(const char *path, guint *count);
static gboolean update_progress(gpointer data);
static gboolean is_supported_audio_file(const char *filename);
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);
static gboolean update_progress_bar(gpointer data);
static void on_progress_bar_clicked(GtkGestureClick *gesture, int n_press, double x, double y, MusicPlayerApp *app);
static void on_next_clicked(GtkButton *button, MusicPlayerApp *app);
static void on_previous_clicked(GtkButton *button, MusicPlayerApp *app);
static void play_next_song(MusicPlayerApp *app);
static void on_about_to_finish(GstElement *playbin, gpointer user_data);
static void on_end_of_stream(GstBus *bus, GstMessage *msg, gpointer user_data);

static void on_folder_selected(GtkButton *button, MusicPlayerApp *app) {
    GtkFileDialog *dialog;
    
    dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Music Folder");
    gtk_file_dialog_set_modal(dialog, TRUE);
    
    gtk_file_dialog_select_folder(dialog,
                                 GTK_WINDOW(app->window),
                                 NULL,
                                 (GAsyncReadyCallback)on_folder_response,
                                 app);
}

static void on_folder_response(GtkNativeDialog *native, GAsyncResult *result, MusicPlayerApp *app) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(native);
    GError *error = NULL;
    GFile *folder = gtk_file_dialog_select_folder_finish(dialog, result, &error);
    
    if (error) {
        g_warning("Error selecting folder: %s", error->message);
        g_error_free(error);
        g_object_unref(dialog);
        return;
    }
    
    if (folder) {
        char *path = g_file_get_path(folder);
        g_print("Selected folder: %s\n", path);
        
        // Show scanning progress bar
        gtk_widget_set_visible(app->progress_bar, TRUE);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.0);
        gtk_label_set_text(GTK_LABEL(app->time_label), "Scanning...");
        
        // Clear existing library
        gtk_list_box_remove_all(GTK_LIST_BOX(app->song_list));
        g_hash_table_remove_all(app->music_library);
        
        app->total_files = 0;
        app->processed_files = 0;
        count_audio_files(path, &app->total_files);
        
        g_print("Found %u audio files\n", app->total_files);  // Debug print
        
        if (app->total_files > 0) {
            g_timeout_add(100, update_progress, app);
            
            scan_music_directory(app, path);
        }
        
        g_free(path);
        g_object_unref(folder);
    }
    
    g_object_unref(dialog);
}

static gboolean is_supported_audio_file(const char *filename) {
    const char *supported_extensions[] = {
        ".mp3", ".flac", ".wav", ".m4a", ".ogg", ".opus", NULL
    };
    
    for (const char **ext = supported_extensions; *ext != NULL; ext++) {
        if (g_str_has_suffix(filename, *ext)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void count_audio_files(const char *path, guint *count) {
    GDir *dir;
    const char *filename;
    GError *error = NULL;

    dir = g_dir_open(path, 0, &error);
    if (error) {
        g_warning("Error counting files in %s: %s", path, error->message);
        g_error_free(error);
        return;
    }

    while ((filename = g_dir_read_name(dir))) {
        char *full_path = g_build_filename(path, filename, NULL);
        
        if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
            count_audio_files(full_path, count);
        } else if (is_supported_audio_file(filename)) {
            (*count)++;
        }
        
        g_free(full_path);
    }
    
    g_dir_close(dir);
}

static gboolean update_progress(gpointer data) {
    MusicPlayerApp *app = data;
    gdouble fraction = (gdouble)app->processed_files / app->total_files;
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), fraction);
    
    if (app->processed_files >= app->total_files) {
        gtk_label_set_text(GTK_LABEL(app->time_label), "0:00 / 0:00");
        return G_SOURCE_REMOVE;
    }
    
    return G_SOURCE_CONTINUE;
}

static void scan_music_directory(MusicPlayerApp *app, const char *path) {
    GDir *dir;
    const char *filename;
    GError *error = NULL;

    dir = g_dir_open(path, 0, &error);
    if (error) {
        g_warning("Error scanning directory %s: %s", path, error->message);
        g_error_free(error);
        return;
    }

    while ((filename = g_dir_read_name(dir))) {
        char *full_path = g_build_filename(path, filename, NULL);
        
        if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
            scan_music_directory(app, full_path);
        } else if (is_supported_audio_file(filename)) {
            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *label = gtk_label_new(filename);
            
            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
            gtk_label_set_xalign(GTK_LABEL(label), 0);
            gtk_widget_set_margin_start(label, 6);
            gtk_widget_set_margin_end(label, 6);
            gtk_widget_set_margin_top(label, 3);
            gtk_widget_set_margin_bottom(label, 3);
            
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
            gtk_list_box_append(GTK_LIST_BOX(app->song_list), row);
            
            g_hash_table_insert(app->music_library, 
                              g_strdup(filename),
                              g_strdup(full_path));
            
            app->processed_files++;
            g_print("Added file: %s\n", filename);  // Debug print
            music_config_add_song(app->config, filename, full_path);
            music_config_save(app->config);
        }
        g_free(full_path);
    }
    g_dir_close(dir);
}

static void on_song_selected(GtkListBox *box, GtkListBoxRow *row, MusicPlayerApp *app) {
    if (!row) return;
    
    GtkWidget *label = gtk_list_box_row_get_child(row);
    const char *filename = gtk_label_get_text(GTK_LABEL(label));
    const char *filepath = g_hash_table_lookup(app->music_library, filename);
    
    if (filepath) {
        if (app->current_file)
            g_free(app->current_file);
        app->current_file = g_strdup(filepath);
        
        gst_element_set_state(app->pipeline, GST_STATE_NULL);
        g_object_set(G_OBJECT(app->source), "location", filepath, NULL);
        
        GstStateChangeReturn ret = gst_element_set_state(app->pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_warning("Failed to start playback");
            return;
        }
        
        app->is_playing = TRUE;
        gtk_button_set_icon_name(GTK_BUTTON(app->play_button), "media-playback-pause");
        
        if (app->progress_id == 0) {
            app->progress_id = g_timeout_add(500, update_progress_bar, app);
        }
        
        if (app->mpris) {
            mpris_update_metadata(app->mpris, filename, filepath);
            mpris_update_playback_status(app->mpris, TRUE);
        }
    }
}

static void on_play_clicked(GtkButton *button, MusicPlayerApp *app) {
    if (!app->current_file) return;
    
    if (app->is_playing) {
        gst_element_set_state(app->pipeline, GST_STATE_PAUSED);
        gtk_button_set_icon_name(GTK_BUTTON(app->play_button), "media-playback-start");
        app->is_playing = FALSE;
    } else {
        GstStateChangeReturn ret = gst_element_set_state(app->pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_warning("Failed to start playback");
            return;
        }
        gtk_button_set_icon_name(GTK_BUTTON(app->play_button), "media-playback-pause");
        app->is_playing = TRUE;
        
        if (app->progress_id == 0) {
            app->progress_id = g_timeout_add(500, update_progress_bar, app);
        }
        
        if (app->mpris) {
            mpris_update_playback_status(app->mpris, app->is_playing);
        }
    }
}

static void on_stop_clicked(GtkButton *button, MusicPlayerApp *app) {
    gst_element_set_state(app->pipeline, GST_STATE_NULL);
    gtk_button_set_icon_name(GTK_BUTTON(app->play_button), "media-playback-start");
    app->is_playing = FALSE;
}

static void on_volume_changed(GtkRange *range, MusicPlayerApp *app) {
    double value = gtk_range_get_value(range);
    g_object_set(G_OBJECT(app->volume), "volume", value / 100.0, NULL);
}

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *sink = GST_ELEMENT(data);
    GstPad *sinkpad = gst_element_get_static_pad(sink, "sink");
    
    if (!sinkpad) {
        g_error("Failed to get sink pad");
        return;
    }
    
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    // Only link if we have audio
    const gchar *mime_type = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    if (g_str_has_prefix(mime_type, "audio/")) {
        GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
        if (GST_PAD_LINK_FAILED(ret)) {
            g_warning("Failed to link pads");
        }
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
    gst_object_unref(sinkpad);
}

static gboolean update_progress_bar(gpointer data) {
    MusicPlayerApp *app = data;
    
    if (!app->is_playing) {
        app->progress_id = 0;
        return G_SOURCE_REMOVE;
    }
    
    gint64 duration, position;
    if (gst_element_query_duration(app->pipeline, GST_FORMAT_TIME, &duration) &&
        gst_element_query_position(app->pipeline, GST_FORMAT_TIME, &position)) {
        
        double progress = (double)position / duration;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), progress);
        
        gchar *time_str = g_strdup_printf("%d:%02d / %d:%02d",
                                         (int)(position / GST_SECOND) / 60,
                                         (int)(position / GST_SECOND) % 60,
                                         (int)(duration / GST_SECOND) / 60,
                                         (int)(duration / GST_SECOND) % 60);
        gtk_label_set_text(GTK_LABEL(app->time_label), time_str);
        g_free(time_str);
    }
    
    return G_SOURCE_CONTINUE;
}

static void seek_to_position(MusicPlayerApp *app, double x, double width) {
    if (!app->is_playing) return;
    
    gint64 duration;
    if (gst_element_query_duration(app->pipeline, GST_FORMAT_TIME, &duration)) {
        double seek_pct = x / width;
        gint64 seek_pos = (gint64)(seek_pct * duration);
        
        if (!gst_element_seek_simple(app->pipeline,
                                   GST_FORMAT_TIME,
                                   GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                                   seek_pos)) {
            g_warning("Seek failed");
        }
    }
}

static void on_progress_bar_clicked(GtkGestureClick *gesture, int n_press, double x, double y, MusicPlayerApp *app) {
    GtkWidget *progress_bar = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    int width = gtk_widget_get_width(progress_bar);
    seek_to_position(app, x, width);
}

static void on_app_shutdown(GtkWidget *window, gpointer user_data);

static void on_activate(GtkApplication *gtk_app, gpointer user_data) {
    MusicPlayerApp *app = g_new0(MusicPlayerApp, 1);
    app->config = music_config_new();
    music_config_load(app->config);
    app->music_library = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    
    app->pipeline = gst_pipeline_new("music-player");
    app->source = gst_element_factory_make("filesrc", "file-source");
    GstElement *decoder = gst_element_factory_make("decodebin", "decoder");
    app->volume = gst_element_factory_make("volume", "volume");
    app->sink = gst_element_factory_make("autoaudiosink", "audio-output");
    
    if (!app->pipeline || !app->source || !decoder || !app->volume || !app->sink) {
        g_error("Failed to create GStreamer elements. Make sure you have gstreamer-plugins-good and gstreamer-plugins-bad installed.");
        return;
    }
    
    gst_bin_add_many(GST_BIN(app->pipeline), app->source, decoder, app->volume, app->sink, NULL);
    gst_element_link(app->source, decoder);
    gst_element_link(app->volume, app->sink);
    
    g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), app->volume);

    app->window = adw_application_window_new(GTK_APPLICATION(gtk_app));
    gtk_window_set_title(GTK_WINDOW(app->window), "Music Player");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 600, 400);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(app->window), main_box);

    GtkWidget *header_bar = adw_header_bar_new();
    GtkWidget *open_button = gtk_button_new_from_icon_name("folder-open");
    adw_header_bar_pack_start(ADW_HEADER_BAR(header_bar), open_button);
    gtk_box_append(GTK_BOX(main_box), header_bar);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    app->song_list = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), app->song_list);
    gtk_box_append(GTK_BOX(main_box), scroll);

    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(controls_box, 12);
    gtk_widget_set_margin_end(controls_box, 12);
    gtk_widget_set_margin_top(controls_box, 12);
    gtk_widget_set_margin_bottom(controls_box, 12);
    
    app->play_button = gtk_button_new_from_icon_name("media-playback-start");
    app->stop_button = gtk_button_new_from_icon_name("media-playback-stop");
    app->prev_button = gtk_button_new_from_icon_name("media-skip-backward");
    app->next_button = gtk_button_new_from_icon_name("media-skip-forward");
    
    GtkAdjustment *volume_adjustment = gtk_adjustment_new(100.0, 0.0, 100.0, 1.0, 10.0, 0.0);
    app->volume_scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, volume_adjustment);
    gtk_widget_set_size_request(app->volume_scale, 100, -1);
    gtk_scale_set_draw_value(GTK_SCALE(app->volume_scale), FALSE);

    gtk_box_append(GTK_BOX(controls_box), app->prev_button);
    gtk_box_append(GTK_BOX(controls_box), app->play_button);
    gtk_box_append(GTK_BOX(controls_box), app->stop_button);
    gtk_box_append(GTK_BOX(controls_box), app->next_button);
    gtk_box_append(GTK_BOX(controls_box), app->volume_scale);

    gtk_box_append(GTK_BOX(main_box), controls_box);

    GtkWidget *progress_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(progress_box, 12);
    gtk_widget_set_margin_end(progress_box, 12);
    gtk_widget_set_margin_bottom(progress_box, 6);
    
    app->progress_bar = gtk_progress_bar_new();
    gtk_widget_set_hexpand(app->progress_bar, TRUE);
    app->time_label = gtk_label_new("0:00 / 0:00");
    
    gtk_box_append(GTK_BOX(progress_box), app->progress_bar);
    gtk_box_append(GTK_BOX(progress_box), app->time_label);
    gtk_box_append(GTK_BOX(main_box), progress_box);

    // Add click handling to progress bar
    GtkGesture *click_gesture = gtk_gesture_click_new();
    g_signal_connect(click_gesture, "pressed", G_CALLBACK(on_progress_bar_clicked), app);
    gtk_widget_add_controller(app->progress_bar, GTK_EVENT_CONTROLLER(click_gesture));

    g_signal_connect(open_button, "clicked", G_CALLBACK(on_folder_selected), app);
    g_signal_connect(app->play_button, "clicked", G_CALLBACK(on_play_clicked), app);
    g_signal_connect(app->stop_button, "clicked", G_CALLBACK(on_stop_clicked), app);
    g_signal_connect(app->volume_scale, "value-changed", G_CALLBACK(on_volume_changed), app);
    g_signal_connect(app->song_list, "row-activated", G_CALLBACK(on_song_selected), app);
    g_signal_connect(app->next_button, "clicked", G_CALLBACK(on_next_clicked), app);
    g_signal_connect(app->prev_button, "clicked", G_CALLBACK(on_previous_clicked), app);
    g_signal_connect(app->pipeline, "about-to-finish", G_CALLBACK(on_about_to_finish), app);

    music_config_load_songs(app->config, app);

    app->mpris = mpris_new(app);

    // Add bus message handling for end-of-stream
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(app->pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::eos", G_CALLBACK(on_end_of_stream), app);
    gst_object_unref(bus);

    // Add about-to-finish signal for gapless playback
    g_signal_connect(app->pipeline, "about-to-finish", G_CALLBACK(on_about_to_finish), app);

    gtk_window_present(GTK_WINDOW(app->window));

    g_signal_connect(app->window, "destroy", G_CALLBACK(on_app_shutdown), app);
}

static void on_app_shutdown(GtkWidget *window, gpointer user_data) {
    MusicPlayerApp *app = (MusicPlayerApp *)user_data;
    
    if (app->pipeline) {
        gst_element_set_state(app->pipeline, GST_STATE_NULL);
        gst_object_unref(app->pipeline);
    }
    
    if (app->current_file)
        g_free(app->current_file);
    
    if (app->music_library)
        g_hash_table_destroy(app->music_library);
    
    if (app->config)
        music_config_free(app->config);
    
    if (app->mpris)
        mpris_free(app->mpris);
    
    g_free(app);
}

static void play_next_song(MusicPlayerApp *app) {
    GtkListBox *list = GTK_LIST_BOX(app->song_list);
    GtkListBoxRow *current = gtk_list_box_get_selected_row(list);
    if (current) {
        int index = gtk_list_box_row_get_index(current);
        GtkListBoxRow *next = gtk_list_box_get_row_at_index(list, index + 1);
        if (next) {
            gtk_list_box_select_row(list, next);
            on_song_selected(list, next, app);
        }
    }
}

static void on_next_clicked(GtkButton *button, MusicPlayerApp *app) {
    play_next_song(app);
}

static void on_previous_clicked(GtkButton *button, MusicPlayerApp *app) {
    GtkListBox *list = GTK_LIST_BOX(app->song_list);
    GtkListBoxRow *current = gtk_list_box_get_selected_row(list);
    if (current) {
        int index = gtk_list_box_row_get_index(current);
        if (index > 0) {
            GtkListBoxRow *prev = gtk_list_box_get_row_at_index(list, index - 1);
            if (prev) {
                gtk_list_box_select_row(list, prev);
                on_song_selected(list, prev, app);
            }
        }
    }
}

static void on_about_to_finish(GstElement *playbin, gpointer user_data) {
    MusicPlayerApp *app = (MusicPlayerApp *)user_data;
    
    // Get the next song's filepath
    GtkListBox *list = GTK_LIST_BOX(app->song_list);
    GtkListBoxRow *current = gtk_list_box_get_selected_row(list);
    if (current) {
        int index = gtk_list_box_row_get_index(current);
        GtkListBoxRow *next = gtk_list_box_get_row_at_index(list, index + 1);
        if (next) {
            GtkWidget *label = gtk_list_box_row_get_child(next);
            const char *filename = gtk_label_get_text(GTK_LABEL(label));
            const char *filepath = g_hash_table_lookup(app->music_library, filename);
            
            if (filepath) {
                // Set up the next song while current is still playing
                g_object_set(G_OBJECT(app->source), "location", filepath, NULL);
            }
        }
    }
}

static void on_end_of_stream(GstBus *bus, GstMessage *msg, gpointer user_data) {
    MusicPlayerApp *app = (MusicPlayerApp *)user_data;
    play_next_song(app);
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    adw_init();
    
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.musicplayer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}