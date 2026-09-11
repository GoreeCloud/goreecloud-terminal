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
#include "theme-engine.h"

#define GOREECLOUD_TERMINAL_APP_ID "com.goreecloud.Terminal.Native"
#define SESSION_STATE_KEY "goreecloud-native-session-state"
#define GLAZE_CSS_RESOURCE "/com/goreecloud/Terminal/Native/glaze-ui.css"

typedef struct {
    GoreeTerminalSessionLifecycle lifecycle;
    GtkWidget *tab_root;
    GtkWidget *tab_text;
    GtkWidget *context_menu;
} TerminalSessionView;

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *theme_button;
    GtkSettings *settings;
    gulong theme_notify_id;
    gboolean system_prefers_dark;
    GoreeTerminalThemeEngine theme_engine;
    guint next_session_id;
} TerminalWindow;

static void add_session(TerminalWindow *terminal_window);
static void apply_theme(TerminalWindow *terminal_window);
static void close_terminal_widget(GtkWidget *terminal);

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
system_uses_high_contrast(GtkSettings *settings)
{
    char *theme_name = NULL;
    gboolean high_contrast = FALSE;

    if (settings == NULL)
        return FALSE;

    g_object_get(settings, "gtk-theme-name", &theme_name, NULL);
    if (theme_name != NULL) {
        char *normalized = g_utf8_strdown(theme_name, -1);
        high_contrast = g_strrstr(normalized, "highcontrast") != NULL ||
                        g_strrstr(normalized, "high-contrast") != NULL;
        g_free(normalized);
    }
    g_free(theme_name);
    return high_contrast;
}

static gboolean
parse_rgba(const char *value, GdkRGBA *rgba)
{
    return value != NULL && gdk_rgba_parse(rgba, value);
}

static void
apply_terminal_palette(TerminalWindow *terminal_window, VteTerminal *terminal)
{
    GoreeTerminalTheme selected = goree_terminal_theme_engine_get(&terminal_window->theme_engine);
    const GoreeTerminalThemeDefinition *theme = goree_terminal_theme_resolve(
        selected,
        terminal_window->system_prefers_dark);
    GdkRGBA foreground;
    GdkRGBA background;
    GdkRGBA cursor;
    GdkRGBA selection;

    if (parse_rgba(theme->foreground, &foreground))
        vte_terminal_set_color_foreground(terminal, &foreground);
    if (parse_rgba(theme->background, &background))
        vte_terminal_set_color_background(terminal, &background);
    if (parse_rgba(theme->cursor, &cursor))
        vte_terminal_set_color_cursor(terminal, &cursor);
    if (parse_rgba(theme->selection_background, &selection))
        vte_terminal_set_color_highlight(terminal, &selection);
}

static void
apply_theme(TerminalWindow *terminal_window)
{
    GtkWidget *window = terminal_window->window;
    GtkWidget *button = terminal_window->theme_button;
    GoreeTerminalTheme selected = goree_terminal_theme_engine_get(&terminal_window->theme_engine);
    const GoreeTerminalThemeDefinition *selected_definition = goree_terminal_theme_definition(selected);
    const GoreeTerminalThemeDefinition *effective = goree_terminal_theme_resolve(
        selected,
        terminal_window->system_prefers_dark);

    gtk_widget_remove_css_class(window, "glaze-system");
    gtk_widget_remove_css_class(window, "glaze-light");
    gtk_widget_remove_css_class(window, "glaze-dark");
    gtk_widget_remove_css_class(window, "glaze-deep-dark");
    gtk_widget_remove_css_class(window, "glaze-high-contrast");

    if (selected == GOREE_TERMINAL_THEME_FOLLOW_SYSTEM)
        gtk_widget_add_css_class(window, "glaze-system");
    else if (selected == GOREE_TERMINAL_THEME_LIGHT)
        gtk_widget_add_css_class(window, "glaze-light");
    else if (selected == GOREE_TERMINAL_THEME_DEEP_DARK)
        gtk_widget_add_css_class(window, "glaze-deep-dark");
    else
        gtk_widget_add_css_class(window, "glaze-dark");

    if (terminal_window->settings != NULL) {
        g_object_set(
            terminal_window->settings,
            "gtk-application-prefer-dark-theme",
            effective->prefers_dark,
            NULL);

        if (system_uses_high_contrast(terminal_window->settings))
            gtk_widget_add_css_class(window, "glaze-high-contrast");
    }

    char *button_label = g_strdup_printf("Theme: %s", selected_definition->label);
    char *accessible_label = g_strdup_printf(
        "Terminal theme: %s",
        selected_definition->label);
    gtk_menu_button_set_label(GTK_MENU_BUTTON(button), button_label);
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(button),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        accessible_label,
        -1);
    gtk_widget_set_tooltip_text(button, accessible_label);
    g_free(button_label);
    g_free(accessible_label);

    int pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal_window->notebook));
    for (int page = 0; page < pages; page++) {
        GtkWidget *terminal = gtk_notebook_get_nth_page(
            GTK_NOTEBOOK(terminal_window->notebook),
            page);
        if (VTE_IS_TERMINAL(terminal))
            apply_terminal_palette(terminal_window, VTE_TERMINAL(terminal));
    }
}

static void
theme_changed(GObject *settings, GParamSpec *pspec, gpointer user_data)
{
    (void) settings;
    (void) pspec;
    apply_theme(user_data);
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
            "Local terminal session %u, exited; output preserved",
            session->lifecycle.id);
        gtk_widget_add_css_class(session->tab_root, "glaze-session-exited");
    } else {
        tab_title = g_strdup_printf("Session %u", session->lifecycle.id);
        accessible_label = g_strdup_printf(
            "Local terminal session %u",
            session->lifecycle.id);
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
close_terminal_widget(GtkWidget *terminal)
{
    GtkWidget *notebook = gtk_widget_get_ancestor(terminal, GTK_TYPE_NOTEBOOK);
    TerminalSessionView *session = session_view_for(terminal);

    if (!GTK_IS_NOTEBOOK(notebook))
        return;

    if (session != NULL)
        goree_terminal_session_request_close(&session->lifecycle);

    int page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), terminal);
    if (page >= 0)
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page);
}

static void
close_session(GtkButton *button, gpointer user_data)
{
    (void) button;
    close_terminal_widget(GTK_WIDGET(user_data));
}

static void
terminal_action_copy(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    vte_terminal_copy_clipboard_format(VTE_TERMINAL(user_data), VTE_FORMAT_TEXT);
}

static void
terminal_action_paste(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    vte_terminal_paste_clipboard(VTE_TERMINAL(user_data));
}

static void
terminal_action_select_all(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    vte_terminal_select_all(VTE_TERMINAL(user_data));
}

static void
terminal_action_clear(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;

    /* Clear only the visible terminal display and home the cursor. This does not
     * execute a shell command and therefore does not alter shell history. */
    vte_terminal_feed(VTE_TERMINAL(user_data), "\033[2J\033[H", -1);
}

static void
terminal_action_close(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    close_terminal_widget(GTK_WIDGET(user_data));
}

static const GActionEntry terminal_actions[] = {
    {"copy", terminal_action_copy, NULL, NULL, NULL, {0, 0, 0}},
    {"paste", terminal_action_paste, NULL, NULL, NULL, {0, 0, 0}},
    {"select-all", terminal_action_select_all, NULL, NULL, NULL, {0, 0, 0}},
    {"clear", terminal_action_clear, NULL, NULL, NULL, {0, 0, 0}},
    {"close", terminal_action_close, NULL, NULL, NULL, {0, 0, 0}},
};

static GtkWidget *
build_terminal_context_menu(GtkWidget *terminal)
{
    GSimpleActionGroup *actions = g_simple_action_group_new();
    g_action_map_add_action_entries(
        G_ACTION_MAP(actions),
        terminal_actions,
        G_N_ELEMENTS(terminal_actions),
        terminal);
    gtk_widget_insert_action_group(terminal, "terminal", G_ACTION_GROUP(actions));
    g_object_unref(actions);

    GMenu *menu = g_menu_new();
    GMenu *edit = g_menu_new();
    GMenu *session = g_menu_new();

    g_menu_append(edit, "Copy", "terminal.copy");
    g_menu_append(edit, "Paste", "terminal.paste");
    g_menu_append(edit, "Select All", "terminal.select-all");
    g_menu_append(edit, "Clear", "terminal.clear");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(edit));

    g_menu_append(session, "New Session", "win.new-session");
    g_menu_append(session, "Close Session", "terminal.close");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(session));

    GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
    gtk_widget_add_css_class(popover, "glaze-context-menu");
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_widget_set_parent(popover, terminal);

    g_object_unref(edit);
    g_object_unref(session);
    g_object_unref(menu);
    return popover;
}

static void
context_menu_pressed(GtkGestureClick *gesture,
                     int n_press,
                     double x,
                     double y,
                     gpointer user_data)
{
    TerminalSessionView *session = user_data;
    GdkRectangle pointing_to = {(int) x, (int) y, 1, 1};

    (void) n_press;
    gtk_popover_set_pointing_to(GTK_POPOVER(session->context_menu), &pointing_to);
    gtk_popover_popup(GTK_POPOVER(session->context_menu));
    gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
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
    gtk_widget_add_css_class(box, "glaze-session-local");
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
    apply_terminal_palette(terminal_window, VTE_TERMINAL(terminal));

    session->context_menu = build_terminal_context_menu(terminal);
    GtkGesture *context_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(context_click), GDK_BUTTON_SECONDARY);
    g_signal_connect(
        context_click,
        "pressed",
        G_CALLBACK(context_menu_pressed),
        session);
    gtk_widget_add_controller(terminal, GTK_EVENT_CONTROLLER(context_click));

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
action_new_session(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    add_session(user_data);
}

static void
action_close_session(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;

    (void) action;
    (void) parameter;

    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal_window->notebook));
    if (page < 0)
        return;

    GtkWidget *terminal = gtk_notebook_get_nth_page(GTK_NOTEBOOK(terminal_window->notebook), page);
    if (terminal != NULL)
        close_terminal_widget(terminal);
}

static void
action_set_theme(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    const char *theme_id = g_variant_get_string(parameter, NULL);

    (void) action;
    if (goree_terminal_theme_engine_set(&terminal_window->theme_engine, theme_id))
        apply_theme(terminal_window);
}

static const GActionEntry window_actions[] = {
    {"new-session", action_new_session, NULL, NULL, NULL, {0, 0, 0}},
    {"close-session", action_close_session, NULL, NULL, NULL, {0, 0, 0}},
    {"set-theme", action_set_theme, "s", NULL, NULL, {0, 0, 0}},
};

static GMenuModel *
build_theme_menu(void)
{
    GMenu *menu = g_menu_new();

    for (int i = 0; i < GOREE_TERMINAL_THEME_COUNT; i++) {
        GoreeTerminalTheme theme = (GoreeTerminalTheme) i;
        GMenuItem *item = g_menu_item_new(goree_terminal_theme_label(theme), NULL);
        g_menu_item_set_action_and_target(
            item,
            "win.set-theme",
            "s",
            goree_terminal_theme_id(theme));
        g_menu_append_item(menu, item);
        g_object_unref(item);
    }

    return G_MENU_MODEL(menu);
}

static void
terminal_window_destroyed(GtkWidget *widget, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;

    (void) widget;

    if (terminal_window->settings != NULL) {
        if (terminal_window->theme_notify_id != 0)
            g_signal_handler_disconnect(
                terminal_window->settings,
                terminal_window->theme_notify_id);

        g_object_set(
            terminal_window->settings,
            "gtk-application-prefer-dark-theme",
            terminal_window->system_prefers_dark,
            NULL);
    }

    g_free(terminal_window);
}

static TerminalWindow *
create_terminal_window(GtkApplication *application)
{
    TerminalWindow *terminal_window = g_new0(TerminalWindow, 1);
    goree_terminal_theme_engine_init(&terminal_window->theme_engine);
    terminal_window->next_session_id = 1;
    terminal_window->settings = gtk_settings_get_default();

    if (terminal_window->settings != NULL) {
        g_object_get(
            terminal_window->settings,
            "gtk-application-prefer-dark-theme",
            &terminal_window->system_prefers_dark,
            NULL);
    }

    GtkWidget *window = gtk_application_window_new(application);
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title = gtk_label_new("GoreeCloud Terminal");
    GtkWidget *new_session = gtk_button_new_from_icon_name("tab-new-symbolic");
    GtkWidget *theme_button = gtk_menu_button_new();
    GtkWidget *notebook = gtk_notebook_new();

    terminal_window->window = window;
    terminal_window->notebook = notebook;
    terminal_window->theme_button = theme_button;

    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_widget_set_size_request(window, 360, 240);
    gtk_widget_add_css_class(window, "glaze-window");
    gtk_widget_add_css_class(header, "glaze-header");
    gtk_widget_add_css_class(title, "title");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), TRUE);

    gtk_widget_add_css_class(new_session, "glaze-action");
    gtk_widget_set_tooltip_text(new_session, "New terminal session (Ctrl+Shift+T)");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(new_session),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "New terminal session",
        -1);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), new_session);

    gtk_widget_add_css_class(theme_button, "glaze-action");
    gtk_widget_add_css_class(theme_button, "glaze-theme-engine");
    GMenuModel *theme_menu = build_theme_menu();
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(theme_button), theme_menu);
    g_object_unref(theme_menu);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), theme_button);

    gtk_widget_add_css_class(notebook, "glaze-session-tabs");
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

    gtk_window_set_titlebar(GTK_WINDOW(window), header);
    gtk_window_set_child(GTK_WINDOW(window), notebook);

    g_action_map_add_action_entries(
        G_ACTION_MAP(window),
        window_actions,
        G_N_ELEMENTS(window_actions),
        terminal_window);

    const char *new_session_accels[] = {"<Primary><Shift>t", NULL};
    const char *close_session_accels[] = {"<Primary><Shift>w", NULL};
    gtk_application_set_accels_for_action(application, "win.new-session", new_session_accels);
    gtk_application_set_accels_for_action(application, "win.close-session", close_session_accels);

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

    if (terminal_window->settings != NULL) {
        terminal_window->theme_notify_id = g_signal_connect(
            terminal_window->settings,
            "notify::gtk-theme-name",
            G_CALLBACK(theme_changed),
            terminal_window);
    }

    apply_theme(terminal_window);
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
