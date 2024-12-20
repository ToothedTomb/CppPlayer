#include <gtk/gtk.h>
#include <gst/gst.h>

// Global variables
GstElement *pipeline = nullptr;         // GStreamer pipeline for playing audio
GtkWidget *play_pause_button;           // Button to toggle between play and pause
GtkWidget *file_label;                  // Label to display the selected file name and type
GtkWidget *volume_slider;               // Slider to control the volume
GtkWidget *volume_label;                // Label for volume control
bool is_playing = false;                // Tracks the play/pause state

// Callback for "Open File" menu item
void on_open_file_activate(GtkMenuItem *menuitem, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open File", GTK_WINDOW(user_data), GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);

    // Run the dialog and check for acceptance
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // If a pipeline already exists, stop it and release resources
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }

        // Create a new pipeline with the selected file
        pipeline = gst_element_factory_make("playbin", "playbin");
        g_object_set(pipeline, "uri", g_strdup_printf("file://%s", filename), NULL);

        // Update the file label to show the selected file's name and type
        char *basename = g_path_get_basename(filename);
        gtk_label_set_text(GTK_LABEL(file_label), basename);
        g_free(basename); // Free the base name memory
        g_free(filename); // Free the filename memory

        // Start playing the file immediately
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        is_playing = true;  // Set the state to playing

        // Update the play/pause button to show the pause icon
        GtkWidget *pause_icon = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(play_pause_button), pause_icon);
    }

    gtk_widget_destroy(dialog); // Close the file dialog
}

// Play/Pause button callback
void on_play_pause_clicked(GtkButton *button, gpointer user_data) {
    if (pipeline) {
        if (is_playing) {
            gst_element_set_state(pipeline, GST_STATE_PAUSED);
            GtkWidget *play_icon = gtk_imselectedage_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
            gtk_button_set_image(GTK_BUTTON(button), play_icon);
            is_playing = false;  // Update the state to not playing
        } else {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
            GtkWidget *pause_icon = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
            gtk_button_set_image(GTK_BUTTON(button), pause_icon);
            is_playing = true;  // Update the state to playing
        }
    }
}

// Volume control callback
void on_volume_changed(GtkRange *range, gpointer user_data) {
    if (pipeline) {
        double volume = gtk_range_get_value(range) / 100.0; // Normalize to 0-1 range
        g_object_set(pipeline, "volume", volume, NULL);  // Set the volume
    }

    // Update the volume label to show the current value
    char volume_text[50];
    snprintf(volume_text, sizeof(volume_text), "Volume: %.0f%%", gtk_range_get_value(range));
    gtk_label_set_text(GTK_LABEL(volume_label), volume_text);
}

// Function to get system volume 
void set_system_volume(GtkScale *scale) {
    double current_volume = 50.0;  
    gtk_range_set_value(GTK_RANGE(scale), current_volume);
    char volume_text[50];
    snprintf(volume_text, sizeof(volume_text), "Volume: %.0f%%", current_volume);
    gtk_label_set_text(GTK_LABEL(volume_label), volume_text);
}

// Restart playback (reset to start)
void on_restart_clicked(GtkButton *button, gpointer user_data) {
    if (pipeline) {
        gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, -1);
    }
}

// Forward/Backward seek callback
void on_seek_forward_clicked(GtkButton *button, gpointer user_data) {
    if (pipeline) {
        gint64 current_pos, duration;
        if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &current_pos) && gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration)) {
            gint64 new_pos = current_pos + 10 * GST_SECOND; // Seek forward 10 seconds
            if (new_pos < duration) {
                gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, new_pos, GST_SEEK_TYPE_NONE, -1);
            }
        }
    }
}

void on_seek_backward_clicked(GtkButton *button, gpointer user_data) {
    if (pipeline) {
        gint64 current_pos, duration;
        if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &current_pos) && gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration)) {
            gint64 new_pos = current_pos - 10 * GST_SECOND; // Seek backward 10 seconds
            if (new_pos > 0) {
                gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, new_pos, GST_SEEK_TYPE_NONE, -1);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    gst_init(&argc, &argv);

    // Create the main application window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "CppPlayer - Music Player");
    gtk_window_set_resizable(GTK_WINDOW(window),false);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    GtkCssProvider *css_background = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_background, "window { background-color: pink; }", -1, NULL);
    GtkStyleContext *style_context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(style_context, GTK_STYLE_PROVIDER(css_background), GTK_STYLE_PROVIDER_PRIORITY_USER);
    // Create a vertical box layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create a menu bar
    GtkWidget *menu_bar = gtk_menu_bar_new();
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_menu_item = gtk_menu_item_new_with_label("File");
    GtkWidget *open_file_item = gtk_menu_item_new_with_label("Open File");

    // Add "Open File" to the "File" menu
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_file_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu_item);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

    // Create a label for the title (with CSS)
    GtkCssProvider *css_provider = gtk_css_provider_new();

    GtkWidget *title_label = gtk_label_new("CppPlayer:");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.5);  // Align to the center
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);
    
    // Create a CSS provider to style the title label
    gtk_css_provider_load_from_data(css_provider, "label { font-size: 44px; font-weight: bold; text-decoration: underline; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Create a label to display the selected file name and type
    file_label = gtk_label_new("No file has been selected");
    gtk_box_pack_start(GTK_BOX(vbox), file_label, FALSE, FALSE, 0);

    // Create a horizontal box for buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);  // Align buttons to center
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

    // Create the Play/Pause button
    play_pause_button = gtk_button_new();
    GtkWidget *play_icon = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(play_pause_button), play_icon); // Set the initial icon
    gtk_box_pack_start(GTK_BOX(button_box), play_pause_button, FALSE, FALSE, 0);
    g_signal_connect(play_pause_button, "clicked", G_CALLBACK(on_play_pause_clicked), NULL);

    // Create Forward and Backward seek buttons
    GtkWidget *seek_backward_button = gtk_button_new_from_icon_name("media-seek-backward", GTK_ICON_SIZE_BUTTON);

    GtkWidget *seek_forward_button = gtk_button_new_from_icon_name("media-seek-forward", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(button_box), seek_backward_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(button_box), seek_forward_button, FALSE, FALSE, 0);
    g_signal_connect(seek_forward_button, "clicked", G_CALLBACK(on_seek_forward_clicked), NULL);
    g_signal_connect(seek_backward_button, "clicked", G_CALLBACK(on_seek_backward_clicked), NULL);

    // Create Restart button
    GtkWidget *restart_button = gtk_button_new();
    GtkWidget *restart_icon = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(restart_button), restart_icon); // Refresh icon
    gtk_box_pack_start(GTK_BOX(button_box), restart_button, FALSE, FALSE, 0);
    g_signal_connect(restart_button, "clicked", G_CALLBACK(on_restart_clicked), NULL);

    // Create volume control label
    volume_label = gtk_label_new("Volume Control: 0%");
    gtk_box_pack_start(GTK_BOX(vbox), volume_label, FALSE, FALSE, 0);

    // Create a volume slider (0 to 100)
    volume_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    gtk_scale_set_value_pos(GTK_SCALE(volume_slider), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(vbox), volume_slider, FALSE, FALSE, 0);
    g_signal_connect(volume_slider, "value-changed", G_CALLBACK(on_volume_changed), NULL);

    // Set initial system volume
    set_system_volume(GTK_SCALE(volume_slider));

    // Connect Open File menu item
    g_signal_connect(open_file_item, "activate", G_CALLBACK(on_open_file_activate), window);

    // Display all widgets and start the GTK main loop
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}

