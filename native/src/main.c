/*
 * GoreeCloud Terminal — native application foundation
 *
 * This source is original GoreeCloud-owned product code. It intentionally does not
 * reuse Ptyxis product architecture, UI code, workflows, or application logic.
 * Mature GTK/VTE platform libraries remain external supporting components.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>

#define GOREECLOUD_TERMINAL_APP_ID "com.goreecloud.Terminal.Native"

static void
spawn_default_shell(VteTerminal *terminal)
{
    const char *shell = g_getenv("SHELL");
    if (shell == NULL || *shell == '\0')
        shell = "/bin/sh";

    char *argv[] = {(char *) shell, NULL};

    vte_terminal_spawn_async(
        terminal,
        VTE_PTY_DEFAULT,
        NULL,
        argv,
        NULL,
        G_SPAWN_DEFAULT,
        NULL,
        NULL,
        NULL,
        -1,
        NULL,
        NULL,
        NULL);
}

static void
activate(GtkApplication *application, gpointer user_data)
{
    (void) user_data;

    GtkWidget *window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Terminal — Native Foundation");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);

    GtkWidget *terminal = vte_terminal_new();
    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(terminal),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Terminal session",
        -1);

    gtk_window_set_child(GTK_WINDOW(window), terminal);
    spawn_default_shell(VTE_TERMINAL(terminal));
    gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char **argv)
{
    GtkApplication *application = gtk_application_new(
        GOREECLOUD_TERMINAL_APP_ID,
        G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(application, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
