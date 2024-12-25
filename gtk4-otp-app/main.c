#include <adwaita.h>
#include <sodium.h>

static void
on_generate_clicked(GtkButton *button, GtkEntry *output_entry)
{
    unsigned char key[32];
    randombytes_buf(key, sizeof key);
    
    char hex_key[65];
    sodium_bin2hex(hex_key, sizeof hex_key, key, sizeof key);
    
    gtk_editable_set_text(GTK_EDITABLE(output_entry), hex_key);
}

static void
app_activate(GApplication *app)
{
    GtkWidget *window = adw_application_window_new(GTK_APPLICATION(app));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *header = adw_header_bar_new();
    GtkWidget *generate_button = gtk_button_new_with_label("Generate Key");
    GtkWidget *output_entry = gtk_entry_new();
    
    gtk_window_set_title(GTK_WINDOW(window), "One-Time Pad Generator");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 100);
    
    gtk_editable_set_editable(GTK_EDITABLE(output_entry), FALSE);
    gtk_widget_set_size_request(generate_button, 120, -1);
    gtk_widget_set_size_request(output_entry, 350, -1);
    
    gtk_widget_set_halign(generate_button, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(output_entry, GTK_ALIGN_CENTER);
    
    gtk_box_append(GTK_BOX(box), header);
    gtk_box_append(GTK_BOX(box), generate_button);
    gtk_box_append(GTK_BOX(box), output_entry);
    
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), box);
    
    g_signal_connect(generate_button, "clicked", G_CALLBACK(on_generate_clicked), output_entry);
    
    gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char *argv[])
{
    if (sodium_init() < 0) {
        return 1;
    }
    
    AdwApplication *app = adw_application_new("org.ivan.OneTimePad", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    
    return g_application_run(G_APPLICATION(app), argc, argv);
} 