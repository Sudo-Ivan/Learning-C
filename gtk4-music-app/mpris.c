#include "mpris.h"
#include "music-app.h"
#include <gtk/gtk.h>
#include <glib/gprintf.h>

static const char *introspection_xml =
    "<node>"
    "  <interface name='org.mpris.MediaPlayer2'>"
    "    <method name='Raise'/>"
    "    <method name='Quit'/>"
    "    <property name='CanQuit' type='b' access='read'/>"
    "    <property name='CanRaise' type='b' access='read'/>"
    "    <property name='Identity' type='s' access='read'/>"
    "    <property name='DesktopEntry' type='s' access='read'/>"
    "  </interface>"
    "  <interface name='org.mpris.MediaPlayer2.Player'>"
    "    <method name='Play'/>"
    "    <method name='Pause'/>"
    "    <method name='PlayPause'/>"
    "    <method name='Stop'/>"
    "    <method name='Next'/>"
    "    <method name='Previous'/>"
    "    <property name='PlaybackStatus' type='s' access='read'/>"
    "    <property name='Metadata' type='a{sv}' access='read'/>"
    "    <property name='Volume' type='d' access='readwrite'/>"
    "    <property name='CanPlay' type='b' access='read'/>"
    "    <property name='CanPause' type='b' access='read'/>"
    "    <property name='CanSeek' type='b' access='read'/>"
    "  </interface>"
    "</node>";

static void handle_method_call(GDBusConnection *connection,
                             const char *sender,
                             const char *object_path,
                             const char *interface_name,
                             const char *method_name,
                             GVariant *parameters,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data) {
    MPRISData *mpris = user_data;
    
    if (g_strcmp0(interface_name, MPRIS_ROOT_INTERFACE) == 0) {
        if (g_strcmp0(method_name, "Quit") == 0) {
            gtk_window_destroy(GTK_WINDOW(mpris->app->window));
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else if (g_strcmp0(method_name, "Raise") == 0) {
            gtk_window_present(GTK_WINDOW(mpris->app->window));
            g_dbus_method_invocation_return_value(invocation, NULL);
        }
    } else if (g_strcmp0(interface_name, MPRIS_PLAYER_INTERFACE) == 0) {
        if (g_strcmp0(method_name, "PlayPause") == 0) {
            gtk_widget_activate(mpris->app->play_button);
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else if (g_strcmp0(method_name, "Play") == 0) {
            if (!mpris->app->is_playing) {
                gtk_widget_activate(mpris->app->play_button);
            }
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else if (g_strcmp0(method_name, "Pause") == 0) {
            if (mpris->app->is_playing) {
                gtk_widget_activate(mpris->app->play_button);
            }
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else if (g_strcmp0(method_name, "Stop") == 0) {
            gtk_widget_activate(mpris->app->stop_button);
            g_dbus_method_invocation_return_value(invocation, NULL);
        }
    }
}

static GVariant* handle_get_property(GDBusConnection *connection,
                                   const char *sender,
                                   const char *object_path,
                                   const char *interface_name,
                                   const char *property_name,
                                   GError **error,
                                   gpointer user_data) {
    MPRISData *mpris = user_data;
    
    if (g_strcmp0(interface_name, MPRIS_ROOT_INTERFACE) == 0) {
        if (g_strcmp0(property_name, "CanQuit") == 0)
            return g_variant_new_boolean(TRUE);
        if (g_strcmp0(property_name, "CanRaise") == 0)
            return g_variant_new_boolean(TRUE);
        if (g_strcmp0(property_name, "Identity") == 0)
            return g_variant_new_string("GTK4 Music Player");
        if (g_strcmp0(property_name, "DesktopEntry") == 0)
            return g_variant_new_string("gtk4-music-app");
    } else if (g_strcmp0(interface_name, MPRIS_PLAYER_INTERFACE) == 0) {
        if (g_strcmp0(property_name, "PlaybackStatus") == 0)
            return g_variant_new_string(mpris->app->is_playing ? "Playing" : "Paused");
        if (g_strcmp0(property_name, "CanPlay") == 0)
            return g_variant_new_boolean(TRUE);
        if (g_strcmp0(property_name, "CanPause") == 0)
            return g_variant_new_boolean(TRUE);
        if (g_strcmp0(property_name, "CanSeek") == 0)
            return g_variant_new_boolean(TRUE);
        if (g_strcmp0(property_name, "Metadata") == 0) {
            GVariantBuilder builder;
            g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
            
            if (mpris->app->current_file) {
                g_variant_builder_add(&builder, "{sv}", "xesam:url",
                                    g_variant_new_string(mpris->app->current_file));
            }
            
            return g_variant_builder_end(&builder);
        }
    }
    
    return NULL;
}

static GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    handle_get_property,
    NULL
};

MPRISData* mpris_new(MusicPlayerApp *app) {
    MPRISData *mpris = g_new0(MPRISData, 1);
    mpris->app = app;
    
    GError *error = NULL;
    mpris->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_warning("Failed to connect to D-Bus: %s", error->message);
        g_error_free(error);
        g_free(mpris);
        return NULL;
    }
    
    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    
    mpris->bus_id = g_bus_own_name_on_connection(mpris->connection,
                                                MPRIS_BUS_NAME_PREFIX "GTK4MusicPlayer",
                                                G_BUS_NAME_OWNER_FLAGS_NONE,
                                                NULL, NULL, NULL, NULL);
    
    mpris->object_id = g_dbus_connection_register_object(mpris->connection,
                                                        MPRIS_OBJECT_PATH,
                                                        introspection_data->interfaces[0],
                                                        &interface_vtable,
                                                        mpris, NULL, &error);
    if (error) {
        g_warning("Failed to register MPRIS object: %s", error->message);
        g_error_free(error);
    }
    
    g_dbus_connection_register_object(mpris->connection,
                                    MPRIS_OBJECT_PATH,
                                    introspection_data->interfaces[1],
                                    &interface_vtable,
                                    mpris, NULL, NULL);
    
    g_dbus_node_info_unref(introspection_data);
    
    return mpris;
}

void mpris_free(MPRISData *mpris) {
    if (!mpris) return;
    
    if (mpris->bus_id)
        g_bus_unown_name(mpris->bus_id);
    
    if (mpris->object_id)
        g_dbus_connection_unregister_object(mpris->connection, mpris->object_id);
    
    if (mpris->connection)
        g_object_unref(mpris->connection);
    
    g_free(mpris);
}

void mpris_update_metadata(MPRISData *mpris, const char *title, const char *path) {
    GVariantBuilder *metadata_builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    
    if (path) {
        g_variant_builder_add(metadata_builder, "{sv}", "xesam:url",
                            g_variant_new_string(path));
    }
    if (title) {
        g_variant_builder_add(metadata_builder, "{sv}", "xesam:title",
                            g_variant_new_string(title));
    }
    
    GVariantBuilder *props_builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(props_builder, "{sv}", "Metadata",
                         g_variant_builder_end(metadata_builder));
    
    GVariant *signal_args = g_variant_new("(sa{sv}as)",
                                         MPRIS_PLAYER_INTERFACE,
                                         props_builder,
                                         NULL);
    
    g_dbus_connection_emit_signal(mpris->connection,
                                NULL,
                                MPRIS_OBJECT_PATH,
                                "org.freedesktop.DBus.Properties",
                                "PropertiesChanged",
                                signal_args,
                                NULL);
    
    g_variant_builder_unref(metadata_builder);
    g_variant_builder_unref(props_builder);
}

void mpris_update_playback_status(MPRISData *mpris, gboolean is_playing) {
    const char *status = is_playing ? "Playing" : "Paused";
    
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder, "{sv}", "PlaybackStatus",
                         g_variant_new_string(status));
    
    GVariant *signal_args = g_variant_new("(sa{sv}as)",
                                         MPRIS_PLAYER_INTERFACE,
                                         builder,
                                         NULL);
    
    g_dbus_connection_emit_signal(mpris->connection,
                                NULL,
                                MPRIS_OBJECT_PATH,
                                "org.freedesktop.DBus.Properties",
                                "PropertiesChanged",
                                signal_args,
                                NULL);
    
    g_variant_builder_unref(builder);
} 