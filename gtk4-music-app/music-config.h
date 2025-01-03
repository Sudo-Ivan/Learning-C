#ifndef MUSIC_CONFIG_H
#define MUSIC_CONFIG_H

#include <glib.h>

// Forward declaration
typedef struct _MusicPlayerApp MusicPlayerApp;

typedef struct {
    GList *music_folders;
    GHashTable *song_library;
} MusicConfig;

MusicConfig* music_config_new(void);
void music_config_free(MusicConfig *config);
gboolean music_config_load(MusicConfig *config);
gboolean music_config_save(MusicConfig *config);
void music_config_add_folder(MusicConfig *config, const char *path);
void music_config_add_song(MusicConfig *config, const char *filename, const char *filepath);
void music_config_load_songs(MusicConfig *config, MusicPlayerApp *app);

#endif 