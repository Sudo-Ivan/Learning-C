#include "music-config.h"
#include "music-app.h"
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>

static char* get_config_path(void) {
    char *config_dir = g_build_filename(g_get_user_config_dir(), "gtk4-music-app", NULL);
    g_mkdir_with_parents(config_dir, 0755);
    char *config_path = g_build_filename(config_dir, "config.json", NULL);
    g_free(config_dir);
    return config_path;
}

MusicConfig* music_config_new(void) {
    MusicConfig *config = g_new0(MusicConfig, 1);
    config->music_folders = NULL;
    config->song_library = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    return config;
}

void music_config_free(MusicConfig *config) {
    if (config) {
        g_list_free_full(config->music_folders, g_free);
        g_hash_table_destroy(config->song_library);
        g_free(config);
    }
}

gboolean music_config_load(MusicConfig *config) {
    char *config_path = get_config_path();
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    
    if (!g_file_test(config_path, G_FILE_TEST_EXISTS)) {
        g_free(config_path);
        g_object_unref(parser);
        return FALSE;
    }
    
    if (!json_parser_load_from_file(parser, config_path, &error)) {
        g_warning("Failed to load config: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        g_free(config_path);
        return FALSE;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);
    
    // Load music folders
    JsonArray *folders = json_object_get_array_member(root_obj, "folders");
    if (folders) {
        guint len = json_array_get_length(folders);
        for (guint i = 0; i < len; i++) {
            const char *folder = json_array_get_string_element(folders, i);
            config->music_folders = g_list_append(config->music_folders, g_strdup(folder));
        }
    }
    
    // Load song library
    JsonObject *library = json_object_get_object_member(root_obj, "library");
    if (library) {
        JsonObjectIter iter;
        const char *key;
        JsonNode *value;
        
        json_object_iter_init(&iter, library);
        while (json_object_iter_next(&iter, &key, &value)) {
            const char *filepath = json_node_get_string(value);
            g_hash_table_insert(config->song_library, g_strdup(key), g_strdup(filepath));
        }
    }
    
    g_object_unref(parser);
    g_free(config_path);
    return TRUE;
}

gboolean music_config_save(MusicConfig *config) {
    char *config_path = get_config_path();
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    
    // Save music folders
    json_builder_set_member_name(builder, "folders");
    json_builder_begin_array(builder);
    for (GList *l = config->music_folders; l; l = l->next) {
        json_builder_add_string_value(builder, (const char *)l->data);
    }
    json_builder_end_array(builder);
    
    // Save song library
    json_builder_set_member_name(builder, "library");
    json_builder_begin_object(builder);
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, config->song_library);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        json_builder_set_member_name(builder, (const char *)key);
        json_builder_add_string_value(builder, (const char *)value);
    }
    json_builder_end_object(builder);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_pretty(gen, TRUE);
    json_generator_set_root(gen, root);
    
    GError *error = NULL;
    if (!json_generator_to_file(gen, config_path, &error)) {
        g_warning("Failed to save config: %s", error->message);
        g_error_free(error);
        g_object_unref(gen);
        json_node_free(root);
        g_object_unref(builder);
        g_free(config_path);
        return FALSE;
    }
    
    g_object_unref(gen);
    json_node_free(root);
    g_object_unref(builder);
    g_free(config_path);
    return TRUE;
}

void music_config_add_folder(MusicConfig *config, const char *path) {
    // Check if folder already exists
    for (GList *l = config->music_folders; l; l = l->next) {
        if (g_strcmp0((const char *)l->data, path) == 0) {
            return;
        }
    }
    config->music_folders = g_list_append(config->music_folders, g_strdup(path));
}

void music_config_add_song(MusicConfig *config, const char *filename, const char *filepath) {
    g_hash_table_insert(config->song_library, g_strdup(filename), g_strdup(filepath));
}

void music_config_load_songs(MusicConfig *config, MusicPlayerApp *app) {
    GHashTableIter iter;
    gpointer key, value;
    
    g_hash_table_iter_init(&iter, config->song_library);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        const char *filename = (const char *)key;
        const char *filepath = (const char *)value;
        
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
                          g_strdup(filepath));
    }
} 