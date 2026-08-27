/*
 * GoreeCloud Terminal — native session/tab/window foundation
 *
 * This source is original GoreeCloud-owned product code. It intentionally does not
 * reuse Ptyxis product architecture, UI code, workflows, or application logic.
 * Mature GTK/VTE platform libraries remain external supporting components.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>

#define GOREECLOUD_TERMINAL_APP_ID "com.goreecloud.Terminal.Native"

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    guint next_session_id;
} TerminalWindow;

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
close_session(GtkButton *button, gpointer user_data)
{
    GtkWidget *session = GTK_WIDGET(user_data);
    GtkWidget *notebook = gtk_widget_get_parent(session);

    (void) button;

    if (!GTK_IS_NOTEBOOK(notebook))
        return;

    int page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), session);
    if (page >= 0)
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page);
}

static GtkWidget *
create_tab_label(GtkWidget *session, guint session_id)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *label = gtk_label_new(NULL);
    GtkWidget *close = gtk_button_new_from_icon_name("window-close-symbolic");
    char *title = g_strdup_printf("Session %u", session_id);

    gtk_label_set_text(GTK_LABEL(label), title);
    gtk_button_set_has_frame(GTK_BUTTON(close), FALSE);
    gtk_widget_set_tooltip_text(close, "Close terminal session");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(close),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Close terminal session",
        -1);

    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), close);
    g_signal_connect(close, "clicked", G_CALLBACK(close_session), session);

    g_free(title);
    return box;
}

static void
add_session(TerminalWindow *terminal_window)
{
    guint session_id = terminal_window->next_session_id++;
    GtkWidget *terminal = vte_terminal_new();
    GtkWidget *tab_label = create_tab_label(terminal, session_id);
    char *accessible_label = g_strdup_printf("Terminal session %u", session_id);

    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(terminal),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        accessible_label,
        -1);

    int page = gtk_notebook_append_page(
        GTK_NOTEBOOK(terminal_window->notebook),
        terminal,
        tab_label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal_window->notebook), page);
    gtk_widget_grab_focus(terminal);

    spawn_default_shell(VTE_TERMINAL(terminal));
    g_free(accessible_label);
}

static void
new_session_clicked(GtkButton *button, gpointer user_data)
{
    (void) button;
    add_session(user_data);
}

static void
terminal_window_destroyed(GtkWidget *widget, gpointer user_data)
{
    (void) widget;
    g_free(user_data);
}

static TerminalWindow *
create_terminal_window(GtkApplication *application)
{
    TerminalWindow *terminal_window = g_new0(TerminalWindow, 1);
    terminal_window->next_session_id = 1;

    GtkWidget *window = gtk_application_window_new(application);
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *new_session = gtk_button_new_from_icon_name("tab-new-symbolic");
    GtkWidget *notebook = gtk_notebook_new();

    terminal_window->window = window;
    terminal_window->notebook = notebook;

    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), TRUE);

    gtk_widget_set_tooltip_text(new_session, "New terminal session");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(new_session),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "New terminal session",
        -1);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), new_session);

    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

    gtk_window_set_titlebar(GTK_WINDOW(window), header);
    gtk_window_set_child(GTK_WINDOW(window), notebook);

    g_signal_connect(
        new_session,
        "clicked",
        G_CALLBACK(new_session_clicked),
        terminal_window);
    g_signal_connect(
        window,
        "destroy",
        G_CALLBACK(terminal_window_destroyed),
        terminal_window);

    add_session(terminal_window);
    return terminal_window;
}

static void
activate(GtkApplication *application, gpointer user_data)
{
    (void) user_data;

    TerminalWindow *terminal_window = create_terminal_window(application);
    gtk_window_present(GTK_WINDOW(terminal_window->window));
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
