/*
 * GoreeCloud Terminal — native session/tab/window foundation
 *
 * This source is original GoreeCloud-owned product code. It intentionally does not
 * reuse Ptyxis product architecture, UI code, workflows, or application logic.
 * Mature GTK/VTE platform libraries remain external supporting components.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>

#include "glaze-contract.h"
#include "session-lifecycle.h"

#define GOREECLOUD_TERMINAL_APP_ID "com.goreecloud.Terminal.Native"
#define SESSION_STATE_KEY "goreecloud-native-session-state"
#define GLAZE_CSS_RESOURCE "/com/goreecloud/Terminal/Native/glaze-ui.css"

typedef struct {
    GoreeTerminalSessionLifecycle lifecycle;
    GtkWidget *tab_root;
    GtkWidget *tab_text;
} TerminalSessionView;

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *appearance_button;
    GoreeTerminalAppearance appearance;
    guint next_session_id;
} TerminalWindow;

static void
install_glaze_ui(void)
{
    static gboolean installed = FALSE;

    if (installed)
        return;

    GdkDisplay *display = gdk_display_get_default();
    if (display == NULL)
        return;

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, GLAZE_CSS_RESOURCE);
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    installed = TRUE;
}

static gboolean
system_uses_high_contrast(void)
{
    GtkSettings *settings = gtk_settings_get_default();
    char *theme_name = NULL;
    gboolean high_contrast = FALSE;

    if (settings == NULL)
        return FALSE;

    g_object_get(settings, "gtk-theme-name", &theme_name, NULL);
    if (theme_name != NULL) {
        high_contrast = g_strrstr(theme_name, "HighContrast") != NULL ||
                        g_strrstr(theme_name, "highcontrast") != NULL;
    }
    g_free(theme_name);
    return high_contrast;
}

static void
apply_appearance(TerminalWindow *terminal_window)
{
    GtkWidget *window = terminal_window->window;
    GtkWidget *button = terminal_window->appearance_button;

    gtk_widget_remove_css_class(window, "glaze-light");
    gtk_widget_remove_css_class(window, "glaze-dark");
    gtk_widget_remove_css_class(window, "glaze-high-contrast");

    if (terminal_window->appearance == GOREE_TERMINAL_APPEARANCE_LIGHT)
        gtk_widget_add_css_class(window, "glaze-light");
    else if (terminal_window->appearance == GOREE_TERMINAL_APPEARANCE_DARK)
        gtk_widget_add_css_class(window, "glaze-dark");

    if (system_uses_high_contrast())
        gtk_widget_add_css_class(window, "glaze-high-contrast");

    gtk_button_set_label(
        GTK_BUTTON(button),
        goree_terminal_appearance_label(terminal_window->appearance));
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(button),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        goree_terminal_appearance_accessible_label(terminal_window->appearance),
        -1);
    gtk_widget_set_tooltip_text(
        button,
        goree_terminal_appearance_accessible_label(terminal_window->appearance));
}

static void
appearance_clicked(GtkButton *button, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;

    (void) button;
    terminal_window->appearance = goree_terminal_appearance_next(terminal_window->appearance);
    apply_appearance(terminal_window);
}

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

static TerminalSessionView *
session_view_for(GtkWidget *terminal)
{
    return g_object_get_data(G_OBJECT(terminal), SESSION_STATE_KEY);
}

static void
update_session_presentation(GtkWidget *terminal, TerminalSessionView *session)
{
    char *tab_title = NULL;
    char *accessible_label = NULL;

    if (session->lifecycle.state == GOREE_TERMINAL_SESSION_EXITED) {
        tab_title = g_strdup_printf("Session %u — Exited", session->lifecycle.id);
        accessible_label = g_strdup_printf(
            "Terminal session %u, exited; output preserved",
            session->lifecycle.id);
        gtk_widget_add_css_class(session->tab_root, "glaze-session-exited");
    } else {
        tab_title = g_strdup_printf("Session %u", session->lifecycle.id);
        accessible_label = g_strdup_printf("Terminal session %u", session->lifecycle.id);
        gtk_widget_remove_css_class(session->tab_root, "glaze-session-exited");
    }

    gtk_label_set_text(GTK_LABEL(session->tab_text), tab_title);
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(terminal),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        accessible_label,
        -1);

    g_free(tab_title);
    g_free(accessible_label);
}

static void
session_child_exited(VteTerminal *terminal, int status, gpointer user_data)
{
    TerminalSessionView *session = user_data;

    goree_terminal_session_mark_child_exited(&session->lifecycle, status);
    update_session_presentation(GTK_WIDGET(terminal), session);
}

static void
close_session(GtkButton *button, gpointer user_data)
{
    GtkWidget *terminal = GTK_WIDGET(user_data);
    GtkWidget *notebook = gtk_widget_get_parent(terminal);
    TerminalSessionView *session = session_view_for(terminal);

    (void) button;

    if (!GTK_IS_NOTEBOOK(notebook))
        return;

    if (session != NULL)
        goree_terminal_session_request_close(&session->lifecycle);

    int page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), terminal);
    if (page >= 0)
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page);
}

static GtkWidget *
create_tab_label(GtkWidget *terminal, TerminalSessionView *session)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *label = gtk_label_new(NULL);
    GtkWidget *close = gtk_button_new_from_icon_name("window-close-symbolic");

    session->tab_root = box;
    session->tab_text = label;
    gtk_widget_add_css_class(box, "glaze-tab-label");
    gtk_widget_add_css_class(close, "glaze-tab-close");
    gtk_button_set_has_frame(GTK_BUTTON(close), FALSE);
    gtk_widget_set_tooltip_text(close, "Close terminal session");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(close),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Close terminal session",
        -1);

    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), close);
    g_signal_connect(close, "clicked", G_CALLBACK(close_session), terminal);
    update_session_presentation(terminal, session);
    return box;
}

static void
add_session(TerminalWindow *terminal_window)
{
    guint session_id = terminal_window->next_session_id++;
    GtkWidget *terminal = vte_terminal_new();
    TerminalSessionView *session = g_new0(TerminalSessionView, 1);

    goree_terminal_session_lifecycle_init(&session->lifecycle, session_id);
    g_object_set_data_full(G_OBJECT(terminal), SESSION_STATE_KEY, session, g_free);

    GtkWidget *tab_label = create_tab_label(terminal, session);
    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);

    int page = gtk_notebook_append_page(
        GTK_NOTEBOOK(terminal_window->notebook),
        terminal,
        tab_label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal_window->notebook), page);
    gtk_widget_grab_focus(terminal);

    g_signal_connect(
        terminal,
        "child-exited",
        G_CALLBACK(session_child_exited),
        session);

    if (goree_terminal_session_mark_running(&session->lifecycle))
        spawn_default_shell(VTE_TERMINAL(terminal));
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
    terminal_window->appearance = GOREE_TERMINAL_APPEARANCE_SYSTEM;
    terminal_window->next_session_id = 1;

    GtkWidget *window = gtk_application_window_new(application);
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("GoreeCloud Terminal");
    GtkWidget *new_session = gtk_button_new_from_icon_name("tab-new-symbolic");
    GtkWidget *appearance = gtk_button_new();
    GtkWidget *notebook = gtk_notebook_new();

    terminal_window->window = window;
    terminal_window->notebook = notebook;
    terminal_window->appearance_button = appearance;

    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_widget_add_css_class(window, "glaze-window");
    gtk_widget_add_css_class(header, "glaze-header");
    gtk_widget_add_css_class(title, "title");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), TRUE);

    gtk_widget_add_css_class(new_session, "glaze-action");
    gtk_widget_set_tooltip_text(new_session, "New terminal session");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(new_session),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "New terminal session",
        -1);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), new_session);

    gtk_widget_add_css_class(appearance, "glaze-action");
    gtk_widget_add_css_class(appearance, "glaze-appearance");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), appearance);

    gtk_widget_add_css_class(notebook, "glaze-session-tabs");
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
        appearance,
        "clicked",
        G_CALLBACK(appearance_clicked),
        terminal_window);
    g_signal_connect(
        window,
        "destroy",
        G_CALLBACK(terminal_window_destroyed),
        terminal_window);

    apply_appearance(terminal_window);
    add_session(terminal_window);
    return terminal_window;
}

static void
activate(GtkApplication *application, gpointer user_data)
{
    (void) user_data;

    install_glaze_ui();
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
