#ifndef MPRIS_H
#define MPRIS_H

#include <gtk/gtk.h>
#include <gio/gio.h>

// Forward declaration
typedef struct _MusicPlayerApp MusicPlayerApp;

#define MPRIS_BUS_NAME_PREFIX "org.mpris.MediaPlayer2."
#define MPRIS_OBJECT_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_ROOT_INTERFACE "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"

typedef struct {
    GDBusConnection *connection;
    guint bus_id;
    guint object_id;
    MusicPlayerApp *app;
} MPRISData;

MPRISData* mpris_new(MusicPlayerApp *app);
void mpris_free(MPRISData *mpris);
void mpris_update_metadata(MPRISData *mpris, const char *title, const char *path);
void mpris_update_playback_status(MPRISData *mpris, gboolean is_playing);

#endif 