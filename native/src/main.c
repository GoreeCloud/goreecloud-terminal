/*
 * GoreeCloud Terminal — native session/tab/window foundation
 *
 * This source is original GoreeCloud-owned product code. Mature GTK/VTE
 * platform libraries remain external supporting components.
 */

#include <glib-unix.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <vte/vte.h>

#include "glaze-contract.h"
#include "host-session-client.h"
#include "paste-guard.h"
#include "profile-runtime.h"
#include "session-lifecycle.h"
#include "terminal-preferences.h"
#include "theme-engine.h"

#define GOREECLOUD_TERMINAL_APP_ID "com.goreecloud.Terminal.Native"
#define SESSION_STATE_KEY "goreecloud-native-session-state"
#define TAB_STATE_KEY "goreecloud-native-tab-state"
#define GLAZE_CSS_RESOURCE "/com/goreecloud/Terminal/Native/glaze-ui.css"

typedef struct _TerminalWindow TerminalWindow;
typedef struct _TerminalTabView TerminalTabView;

typedef enum {
    TERMINAL_SESSION_LOCAL_HOST,
    TERMINAL_SESSION_LOCAL_UNAVAILABLE,
    TERMINAL_SESSION_HOST_BRIDGE,
    TERMINAL_SESSION_HOST_UNAVAILABLE,
} TerminalSessionOrigin;

typedef struct {
    GoreeTerminalSessionLifecycle lifecycle;
    GoreeTerminalHostSession host_session;
    GtkWidget *terminal;
    GtkWidget *pane_root;
    GtkWidget *context_menu;
    GtkWidget *search_popover;
    GtkWidget *search_entry;
    TerminalWindow *owner;
    TerminalTabView *tab;
    const GoreeTerminalSessionProfile *profile;
    TerminalSessionOrigin origin;
    guint host_watch_id;
} TerminalSessionView;

struct _TerminalTabView {
    TerminalWindow *owner;
    GtkWidget *page_root;
    GtkWidget *tab_root;
    GtkWidget *tab_text;
    GtkWidget *tab_menu;
    GPtrArray *sessions;
    TerminalSessionView *active_session;
    char *default_title;
    char *custom_title;
};

typedef struct {
    GtkWidget *terminal;
    GoreeTerminalPasteProtection protection;
} PasteReadRequest;

typedef struct {
    GtkWidget *terminal;
    char *text;
} PasteConfirmRequest;

struct _TerminalWindow {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *theme_button;
    GtkWidget *open_tabs_button;
    GtkWidget *menu_button;
    GtkSettings *settings;
    gulong theme_notify_id;
    gboolean system_prefers_dark;
    GoreeTerminalThemeEngine theme_engine;
    GoreeTerminalPreferences preferences;
    GoreeTerminalRuntimeCatalog catalog;
    gboolean catalog_ready;
    char *catalog_error;
    guint next_session_id;
};

static void add_session(TerminalWindow *terminal_window);
static void apply_theme(TerminalWindow *terminal_window);
static void close_terminal_widget(GtkWidget *terminal);
static void update_open_tabs_menu(TerminalWindow *terminal_window);
static void update_session_presentation(GtkWidget *terminal, TerminalSessionView *session);
static void update_tab_presentation(TerminalTabView *tab);
static TerminalWindow *create_terminal_window(GtkApplication *application);

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

static GoreeTerminalTheme
profile_terminal_theme(TerminalWindow *terminal_window,
                       const GoreeTerminalSessionProfile *profile)
{
    GoreeTerminalTheme selected = goree_terminal_theme_engine_get(
        &terminal_window->theme_engine);

    if (profile == NULL || profile->theme_id == NULL ||
        *profile->theme_id == '\0' ||
        g_str_equal(profile->theme_id, "follow-system"))
        return selected;

    for (int index = 0; index < GOREE_TERMINAL_THEME_COUNT; index++) {
        GoreeTerminalTheme candidate = (GoreeTerminalTheme) index;
        if (g_strcmp0(profile->theme_id,
                      goree_terminal_theme_id(candidate)) == 0)
            return candidate;
    }
    return selected;
}

static TerminalSessionView *
session_view_for(GtkWidget *terminal)
{
    if (!VTE_IS_TERMINAL(terminal))
        return NULL;
    return g_object_get_data(G_OBJECT(terminal), SESSION_STATE_KEY);
}

static void
apply_terminal_palette(TerminalWindow *terminal_window, VteTerminal *terminal)
{
    TerminalSessionView *session = session_view_for(GTK_WIDGET(terminal));
    GoreeTerminalTheme selected = profile_terminal_theme(
        terminal_window,
        session != NULL ? session->profile : NULL);
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
apply_terminal_preferences(TerminalWindow *terminal_window,
                           VteTerminal *terminal,
                           const GoreeTerminalSessionProfile *profile)
{
    guint scrollback_lines = terminal_window->preferences.scrollback_lines;
    if (profile != NULL)
        scrollback_lines = profile->scrollback_lines;

    vte_terminal_set_scrollback_lines(terminal, (glong) scrollback_lines);
    vte_terminal_set_audible_bell(
        terminal,
        terminal_window->preferences.audible_bell);
    vte_terminal_set_allow_hyperlink(
        terminal,
        terminal_window->preferences.allow_hyperlinks);
    vte_terminal_search_set_wrap_around(
        terminal,
        terminal_window->preferences.search_wrap_around);
}

typedef void (*TerminalVisitor)(VteTerminal *terminal, gpointer user_data);

static void
visit_terminals(GtkWidget *root, TerminalVisitor visitor, gpointer user_data)
{
    if (root == NULL)
        return;
    if (VTE_IS_TERMINAL(root)) {
        visitor(VTE_TERMINAL(root), user_data);
        return;
    }

    for (GtkWidget *child = gtk_widget_get_first_child(root);
         child != NULL;
         child = gtk_widget_get_next_sibling(child))
        visit_terminals(child, visitor, user_data);
}

static void
apply_palette_visitor(VteTerminal *terminal, gpointer user_data)
{
    apply_terminal_palette(user_data, terminal);
}

static GtkWidget *
first_terminal_in_widget(GtkWidget *root)
{
    if (root == NULL)
        return NULL;
    if (VTE_IS_TERMINAL(root))
        return root;

    for (GtkWidget *child = gtk_widget_get_first_child(root);
         child != NULL;
         child = gtk_widget_get_next_sibling(child)) {
        GtkWidget *terminal = first_terminal_in_widget(child);
        if (terminal != NULL)
            return terminal;
    }
    return NULL;
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
        GtkWidget *page_root = gtk_notebook_get_nth_page(
            GTK_NOTEBOOK(terminal_window->notebook),
            page);
        visit_terminals(page_root, apply_palette_visitor, terminal_window);
    }
}

static void
theme_changed(GObject *settings, GParamSpec *pspec, gpointer user_data)
{
    (void) settings;
    (void) pspec;
    apply_theme(user_data);
}

static gboolean
running_in_flatpak(void)
{
    return g_file_test("/.flatpak-info", G_FILE_TEST_EXISTS);
}

static void
append_environment(GPtrArray *environment,
                   GHashTable *names,
                   const char *name,
                   const char *value)
{
    if (name == NULL || value == NULL || g_hash_table_contains(names, name))
        return;

    g_ptr_array_add(environment, g_strdup_printf("%s=%s", name, value));
    g_hash_table_add(names, (gpointer) name);
}

static char **
build_local_environment(const GoreeTerminalSessionProfile *profile,
                        const char *shell,
                        const char *working_directory)
{
    static const char *const inherited_safe[] = {
        "LANG", "LC_ALL", "LC_CTYPE", "LC_MESSAGES", "TZ", NULL
    };
    GPtrArray *environment = g_ptr_array_new_with_free_func(g_free);
    GHashTable *names = g_hash_table_new(g_str_hash, g_str_equal);
    const char *path = g_getenv("PATH");

    append_environment(environment, names, "HOME", g_get_home_dir());
    append_environment(environment, names, "USER", g_get_user_name());
    append_environment(environment, names, "LOGNAME", g_get_user_name());
    append_environment(environment, names, "SHELL", shell);
    append_environment(environment, names, "PWD", working_directory);
    append_environment(environment, names, "TERM", "xterm-256color");
    append_environment(environment, names, "COLORTERM", "truecolor");
    append_environment(
        environment,
        names,
        "PATH",
        path != NULL ? path : "/usr/local/bin:/usr/bin:/bin");

    if (profile->environment_policy == GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE) {
        for (guint index = 0; inherited_safe[index] != NULL; index++)
            append_environment(
                environment,
                names,
                inherited_safe[index],
                g_getenv(inherited_safe[index]));
    }

    if (profile->environment_allowlist != NULL) {
        for (char **name = profile->environment_allowlist; *name != NULL; name++)
            append_environment(environment, names, *name, g_getenv(*name));
    }

    g_hash_table_unref(names);
    g_ptr_array_add(environment, NULL);
    return (char **) g_ptr_array_free(environment, FALSE);
}

static const char *
profile_shell(const GoreeTerminalSessionProfile *profile)
{
    if (profile != NULL && profile->shell_path != NULL &&
        *profile->shell_path != '\0')
        return profile->shell_path;

    const char *shell = g_getenv("SHELL");
    return shell != NULL && *shell != '\0' ? shell : "/bin/sh";
}

static const char *
profile_working_directory(const GoreeTerminalSessionProfile *profile)
{
    if (profile != NULL && profile->working_directory != NULL &&
        *profile->working_directory != '\0')
        return profile->working_directory;
    return g_get_home_dir();
}

static void
local_spawn_ready(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data)
{
    TerminalSessionView *session = user_data;

    (void) pid;
    if (error == NULL)
        return;

    session->origin = TERMINAL_SESSION_LOCAL_UNAVAILABLE;
    goree_terminal_session_mark_disconnected(&session->lifecycle);
    vte_terminal_set_input_enabled(terminal, FALSE);
    vte_terminal_feed(
        terminal,
        "\r\nGoreeCloud Terminal could not start the selected local profile.\r\n"
        "No fallback command was executed. Review the profile shell and working directory.\r\n",
        -1);
    update_session_presentation(GTK_WIDGET(terminal), session);
}

static void
spawn_profile_shell(TerminalSessionView *session)
{
    const char *shell = profile_shell(session->profile);
    const char *working_directory = profile_working_directory(session->profile);
    char *argv[] = {(char *) shell, NULL};
    char **environment = build_local_environment(
        session->profile,
        shell,
        working_directory);

    vte_terminal_spawn_async(
        VTE_TERMINAL(session->terminal),
        VTE_PTY_DEFAULT,
        working_directory,
        argv,
        environment,
        G_SPAWN_DEFAULT,
        NULL,
        NULL,
        NULL,
        -1,
        NULL,
        local_spawn_ready,
        session);
    g_strfreev(environment);
}

static void
session_view_free(gpointer data)
{
    TerminalSessionView *session = data;

    if (session == NULL)
        return;

    if (session->host_watch_id != 0) {
        g_source_remove(session->host_watch_id);
        session->host_watch_id = 0;
    }
    goree_terminal_host_session_close(&session->host_session);
    g_free(session);
}

static void
tab_view_free(gpointer data)
{
    TerminalTabView *tab = data;

    if (tab == NULL)
        return;
    g_clear_pointer(&tab->sessions, g_ptr_array_unref);
    g_free(tab->default_title);
    g_free(tab->custom_title);
    g_free(tab);
}

static const char *
session_origin_description(const TerminalSessionView *session)
{
    switch (session->origin) {
    case TERMINAL_SESSION_HOST_BRIDGE:
        return "verified local host session";
    case TERMINAL_SESSION_HOST_UNAVAILABLE:
        return "host session unavailable";
    case TERMINAL_SESSION_LOCAL_UNAVAILABLE:
        return "local session unavailable";
    case TERMINAL_SESSION_LOCAL_HOST:
    default:
        return "local host terminal session";
    }
}

static void
update_tab_presentation(TerminalTabView *tab)
{
    if (tab == NULL || tab->tab_text == NULL)
        return;

    TerminalSessionView *session = tab->active_session;
    const char *base_title = tab->custom_title != NULL && *tab->custom_title != '\0'
        ? tab->custom_title
        : tab->default_title;
    if (base_title == NULL || *base_title == '\0')
        base_title = "Terminal";

    char *title = NULL;
    if (session != NULL &&
        session->lifecycle.state == GOREE_TERMINAL_SESSION_DISCONNECTED)
        title = g_strdup_printf("%s — Disconnected", base_title);
    else if (session != NULL &&
             session->lifecycle.state == GOREE_TERMINAL_SESSION_EXITED)
        title = g_strdup_printf("%s — Exited", base_title);
    else
        title = g_strdup(base_title);

    gtk_widget_remove_css_class(tab->tab_root, "glaze-session-local");
    gtk_widget_remove_css_class(tab->tab_root, "glaze-session-host");
    gtk_widget_remove_css_class(tab->tab_root, "glaze-session-disconnected");
    gtk_widget_remove_css_class(tab->tab_root, "glaze-session-exited");

    if (session != NULL) {
        if (session->origin == TERMINAL_SESSION_HOST_BRIDGE)
            gtk_widget_add_css_class(tab->tab_root, "glaze-session-host");
        else if (session->origin == TERMINAL_SESSION_LOCAL_HOST)
            gtk_widget_add_css_class(tab->tab_root, "glaze-session-local");

        if (session->lifecycle.state == GOREE_TERMINAL_SESSION_DISCONNECTED)
            gtk_widget_add_css_class(tab->tab_root, "glaze-session-disconnected");
        else if (session->lifecycle.state == GOREE_TERMINAL_SESSION_EXITED)
            gtk_widget_add_css_class(tab->tab_root, "glaze-session-exited");
    }

    gtk_editable_set_text(GTK_EDITABLE(tab->tab_text), title);

    if (session != NULL) {
        const char *profile_name = session->profile != NULL
            ? session->profile->name
            : "Default";
        char *tooltip = g_strdup_printf(
            "%s; active pane: %s; %s",
            title,
            profile_name,
            session_origin_description(session));
        gtk_widget_set_tooltip_text(tab->tab_root, tooltip);
        g_free(tooltip);
    } else {
        gtk_widget_set_tooltip_text(tab->tab_root, title);
    }
    g_free(title);

    if (tab->owner != NULL)
        update_open_tabs_menu(tab->owner);
}

static void
update_session_presentation(GtkWidget *terminal, TerminalSessionView *session)
{
    const char *origin_description = session_origin_description(session);
    const char *profile_name = session->profile != NULL
        ? session->profile->name
        : "Default";
    char *accessible_label;

    if (session->lifecycle.state == GOREE_TERMINAL_SESSION_DISCONNECTED) {
        accessible_label = g_strdup_printf(
            "%s profile, %s %u, disconnected; input disabled and output preserved",
            profile_name,
            origin_description,
            session->lifecycle.id);
    } else if (session->lifecycle.state == GOREE_TERMINAL_SESSION_EXITED) {
        accessible_label = g_strdup_printf(
            "%s profile, %s %u, exited; output preserved",
            profile_name,
            origin_description,
            session->lifecycle.id);
    } else {
        accessible_label = g_strdup_printf(
            "%s profile, %s %u",
            profile_name,
            origin_description,
            session->lifecycle.id);
    }

    gtk_accessible_update_property(
        GTK_ACCESSIBLE(terminal),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        accessible_label,
        -1);
    gtk_widget_set_tooltip_text(session->pane_root, accessible_label);
    g_free(accessible_label);

    if (session->tab != NULL && session->tab->active_session == session)
        update_tab_presentation(session->tab);
}

static void
session_focus_entered(GtkEventControllerFocus *controller, gpointer user_data)
{
    TerminalSessionView *session = user_data;
    TerminalTabView *tab = session->tab;

    (void) controller;
    if (tab == NULL)
        return;

    for (guint index = 0; index < tab->sessions->len; index++) {
        TerminalSessionView *candidate = g_ptr_array_index(tab->sessions, index);
        if (candidate != NULL && candidate->pane_root != NULL)
            gtk_widget_remove_css_class(candidate->pane_root, "glaze-active-pane");
    }
    gtk_widget_add_css_class(session->pane_root, "glaze-active-pane");
    tab->active_session = session;
    update_tab_presentation(tab);
}

static void
session_child_exited(VteTerminal *terminal, int status, gpointer user_data)
{
    TerminalSessionView *session = user_data;

    if (session->origin != TERMINAL_SESSION_LOCAL_HOST)
        return;

    goree_terminal_session_mark_child_exited(&session->lifecycle, status);
    vte_terminal_set_input_enabled(terminal, FALSE);
    update_session_presentation(GTK_WIDGET(terminal), session);
}

static gboolean
host_control_ready(gint fd, GIOCondition condition, gpointer user_data)
{
    TerminalSessionView *session = user_data;
    GoreeTerminalHostEvent event;
    GError *error = NULL;
    int value = 0;

    (void) fd;

    if ((condition & G_IO_NVAL) != 0) {
        event = GOREE_TERMINAL_HOST_EVENT_DISCONNECTED;
    } else {
        event = goree_terminal_host_session_poll_event(
            &session->host_session,
            &value,
            &error);
        if (event == GOREE_TERMINAL_HOST_EVENT_NONE &&
            (condition & (G_IO_HUP | G_IO_ERR)) != 0)
            event = GOREE_TERMINAL_HOST_EVENT_DISCONNECTED;
    }

    if (event == GOREE_TERMINAL_HOST_EVENT_NONE) {
        g_clear_error(&error);
        return G_SOURCE_CONTINUE;
    }

    session->host_watch_id = 0;
    goree_terminal_host_session_close(&session->host_session);

    if (event == GOREE_TERMINAL_HOST_EVENT_EXITED) {
        goree_terminal_session_mark_child_exited(&session->lifecycle, value);
    } else {
        goree_terminal_session_mark_disconnected(&session->lifecycle);
        session->origin = TERMINAL_SESSION_HOST_UNAVAILABLE;
    }

    vte_terminal_set_input_enabled(VTE_TERMINAL(session->terminal), FALSE);
    update_session_presentation(session->terminal, session);
    g_clear_error(&error);
    return G_SOURCE_REMOVE;
}

static gboolean
start_host_bridge_session(TerminalSessionView *session)
{
    GError *error = NULL;
    int pty_fd;
    VtePty *pty;
    GoreeTerminalHostLaunchContext context = {0};

    goree_terminal_runtime_launch_context_for_profile(session->profile, &context);
    if (!goree_terminal_host_session_connect_with_context(
            &session->host_session,
            24,
            80,
            &context,
            &error)) {
        g_clear_error(&error);
        return FALSE;
    }

    pty_fd = goree_terminal_host_session_steal_pty_fd(&session->host_session);
    pty = vte_pty_new_foreign_sync(pty_fd, NULL, &error);
    if (pty == NULL) {
        close(pty_fd);
        goree_terminal_host_session_close(&session->host_session);
        g_clear_error(&error);
        return FALSE;
    }

    vte_terminal_set_pty(VTE_TERMINAL(session->terminal), pty);
    g_object_unref(pty);

    if (!goree_terminal_session_mark_running(&session->lifecycle)) {
        vte_terminal_set_pty(VTE_TERMINAL(session->terminal), NULL);
        goree_terminal_host_session_close(&session->host_session);
        return FALSE;
    }

    session->origin = TERMINAL_SESSION_HOST_BRIDGE;
    vte_terminal_set_input_enabled(VTE_TERMINAL(session->terminal), TRUE);
    session->host_watch_id = g_unix_fd_add(
        goree_terminal_host_session_control_fd(&session->host_session),
        G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
        host_control_ready,
        session);
    if (session->host_watch_id == 0) {
        vte_terminal_set_input_enabled(VTE_TERMINAL(session->terminal), FALSE);
        vte_terminal_set_pty(VTE_TERMINAL(session->terminal), NULL);
        goree_terminal_host_session_close(&session->host_session);
        goree_terminal_session_mark_disconnected(&session->lifecycle);
        return FALSE;
    }

    return TRUE;
}

static void
mark_host_session_unavailable(TerminalSessionView *session)
{
    static const char message[] =
        "\r\nGoreeCloud Terminal host session is unavailable.\r\n"
        "The sandboxed application will not silently substitute a sandbox shell.\r\n"
        "Start or repair the GoreeCloud Terminal host-session service, then open a new session.\r\n";

    session->origin = TERMINAL_SESSION_HOST_UNAVAILABLE;
    goree_terminal_session_mark_disconnected(&session->lifecycle);
    vte_terminal_set_input_enabled(VTE_TERMINAL(session->terminal), FALSE);
    vte_terminal_feed(VTE_TERMINAL(session->terminal), message, -1);
    update_session_presentation(session->terminal, session);
}

static void
close_tab_view(TerminalTabView *tab)
{
    if (tab == NULL || tab->owner == NULL || tab->page_root == NULL)
        return;

    for (guint index = 0; index < tab->sessions->len; index++) {
        TerminalSessionView *session = g_ptr_array_index(tab->sessions, index);
        if (session != NULL)
            goree_terminal_session_request_close(&session->lifecycle);
    }

    int page = gtk_notebook_page_num(
        GTK_NOTEBOOK(tab->owner->notebook),
        tab->page_root);
    if (page >= 0)
        gtk_notebook_remove_page(GTK_NOTEBOOK(tab->owner->notebook), page);
    update_open_tabs_menu(tab->owner);
}

static void
close_terminal_widget(GtkWidget *terminal)
{
    TerminalSessionView *session = session_view_for(terminal);
    if (session != NULL)
        close_tab_view(session->tab);
}

static GtkWindow *
terminal_parent_window(GtkWidget *terminal)
{
    GtkRoot *root = gtk_widget_get_root(terminal);
    return GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : NULL;
}

static gboolean
terminal_can_accept_paste(GtkWidget *terminal)
{
    TerminalSessionView *session = session_view_for(terminal);
    return session != NULL &&
           goree_terminal_session_can_accept_input(&session->lifecycle) &&
           vte_terminal_get_input_enabled(VTE_TERMINAL(terminal));
}

static void
show_simple_alert(GtkWidget *terminal, const char *message, const char *detail)
{
    GtkAlertDialog *dialog = gtk_alert_dialog_new("%s", message);
    if (detail != NULL)
        gtk_alert_dialog_set_detail(dialog, detail);
    gtk_alert_dialog_show(dialog, terminal_parent_window(terminal));
    g_object_unref(dialog);
}

static void
paste_confirm_request_free(PasteConfirmRequest *request)
{
    if (request == NULL)
        return;
    g_clear_object(&request->terminal);
    g_free(request->text);
    g_free(request);
}

static void
paste_confirmation_ready(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    PasteConfirmRequest *request = user_data;
    GError *error = NULL;
    int choice = gtk_alert_dialog_choose_finish(
        GTK_ALERT_DIALOG(source_object),
        result,
        &error);

    if (error == NULL && choice == 1 && terminal_can_accept_paste(request->terminal))
        vte_terminal_paste_text(VTE_TERMINAL(request->terminal), request->text);

    g_clear_error(&error);
    paste_confirm_request_free(request);
}

static void
confirm_guarded_paste(
    GtkWidget *terminal,
    char *text,
    const GoreeTerminalPasteAssessment *assessment)
{
    const char *buttons[] = {"Cancel", "Paste", NULL};
    GtkAlertDialog *dialog = gtk_alert_dialog_new("Paste clipboard text into this terminal?");
    char *detail = assessment->contains_control_characters
        ? g_strdup_printf(
            "The clipboard contains %u line(s) and control characters. Its contents are not displayed or logged. Confirm before sending it to the active session.",
            assessment->line_count)
        : g_strdup_printf(
            "The clipboard contains %u line(s). Its contents are not displayed or logged. Confirm before sending it to the active session.",
            assessment->line_count);

    gtk_alert_dialog_set_detail(dialog, detail);
    gtk_alert_dialog_set_buttons(dialog, buttons);
    gtk_alert_dialog_set_cancel_button(dialog, 0);
    gtk_alert_dialog_set_default_button(dialog, 0);
    gtk_alert_dialog_set_modal(dialog, TRUE);
    g_free(detail);

    PasteConfirmRequest *request = g_new0(PasteConfirmRequest, 1);
    request->terminal = g_object_ref(terminal);
    request->text = text;

    gtk_alert_dialog_choose(
        dialog,
        terminal_parent_window(terminal),
        NULL,
        paste_confirmation_ready,
        request);
    g_object_unref(dialog);
}

static void
clipboard_text_ready(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    PasteReadRequest *request = user_data;
    GError *error = NULL;
    char *text = gdk_clipboard_read_text_finish(
        GDK_CLIPBOARD(source_object),
        result,
        &error);

    if (error != NULL || text == NULL) {
        if (terminal_can_accept_paste(request->terminal))
            show_simple_alert(
                request->terminal,
                "Clipboard text is unavailable",
                "GoreeCloud Terminal did not send any clipboard contents to the terminal session.");
        g_clear_error(&error);
        g_free(text);
        g_clear_object(&request->terminal);
        g_free(request);
        return;
    }

    GoreeTerminalPasteAssessment assessment = goree_terminal_assess_paste(
        text,
        request->protection);

    if (!terminal_can_accept_paste(request->terminal)) {
        g_free(text);
    } else if (assessment.decision == GOREE_TERMINAL_PASTE_REJECT_INVALID_TEXT) {
        show_simple_alert(
            request->terminal,
            "Clipboard text cannot be pasted",
            "The clipboard is not valid UTF-8 text. No clipboard contents were sent to the terminal session.");
        g_free(text);
    } else if (assessment.decision == GOREE_TERMINAL_PASTE_REQUIRES_CONFIRMATION) {
        confirm_guarded_paste(request->terminal, text, &assessment);
    } else {
        vte_terminal_paste_text(VTE_TERMINAL(request->terminal), text);
        g_free(text);
    }

    g_clear_object(&request->terminal);
    g_free(request);
}

static void
begin_guarded_paste(GtkWidget *terminal)
{
    TerminalSessionView *session = session_view_for(terminal);
    if (session == NULL || session->owner == NULL || !terminal_can_accept_paste(terminal))
        return;

    GdkDisplay *display = gtk_widget_get_display(terminal);
    if (display == NULL)
        return;

    PasteReadRequest *request = g_new0(PasteReadRequest, 1);
    request->terminal = g_object_ref(terminal);
    request->protection = session->owner->preferences.paste_protection;

    gdk_clipboard_read_text_async(
        gdk_display_get_clipboard(display),
        NULL,
        clipboard_text_ready,
        request);
}

static void
set_terminal_search(TerminalSessionView *session, const char *text)
{
    if (text == NULL || *text == '\0') {
        vte_terminal_search_set_regex(VTE_TERMINAL(session->terminal), NULL, 0);
        return;
    }

    char *escaped = g_regex_escape_string(text, -1);
    GError *error = NULL;
    VteRegex *regex = vte_regex_new_for_search(escaped, -1, 0, &error);
    g_free(escaped);

    if (regex == NULL) {
        gtk_widget_set_tooltip_text(session->search_entry, "Search text could not be prepared.");
        g_clear_error(&error);
        return;
    }

    gtk_widget_set_tooltip_text(session->search_entry, NULL);
    vte_terminal_search_set_regex(VTE_TERMINAL(session->terminal), regex, 0);
    vte_regex_unref(regex);
    g_clear_error(&error);
}

static void
search_changed(GtkSearchEntry *entry, gpointer user_data)
{
    TerminalSessionView *session = user_data;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    set_terminal_search(session, text);
    if (text != NULL && *text != '\0')
        vte_terminal_search_find_next(VTE_TERMINAL(session->terminal));
}

static void
search_next_clicked(GtkButton *button, gpointer user_data)
{
    (void) button;
    TerminalSessionView *session = user_data;
    vte_terminal_search_find_next(VTE_TERMINAL(session->terminal));
}

static void
search_previous_clicked(GtkButton *button, gpointer user_data)
{
    (void) button;
    TerminalSessionView *session = user_data;
    vte_terminal_search_find_previous(VTE_TERMINAL(session->terminal));
}

static void
search_close_clicked(GtkButton *button, gpointer user_data)
{
    (void) button;
    TerminalSessionView *session = user_data;
    gtk_popover_popdown(GTK_POPOVER(session->search_popover));
    gtk_widget_grab_focus(session->terminal);
}

static GtkWidget *
build_search_popover(TerminalSessionView *session)
{
    GtkWidget *popover = gtk_popover_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *entry = gtk_search_entry_new();
    GtkWidget *previous = gtk_button_new_from_icon_name("go-up-symbolic");
    GtkWidget *next = gtk_button_new_from_icon_name("go-down-symbolic");
    GtkWidget *close = gtk_button_new_from_icon_name("window-close-symbolic");

    session->search_entry = entry;
    gtk_widget_set_size_request(entry, 240, -1);
    gtk_widget_set_tooltip_text(previous, "Previous match");
    gtk_widget_set_tooltip_text(next, "Next match");
    gtk_widget_set_tooltip_text(close, "Close search");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(entry),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Search terminal output",
        -1);

    gtk_box_append(GTK_BOX(box), entry);
    gtk_box_append(GTK_BOX(box), previous);
    gtk_box_append(GTK_BOX(box), next);
    gtk_box_append(GTK_BOX(box), close);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), TRUE);
    gtk_widget_add_css_class(popover, "glaze-context-menu");
    gtk_widget_set_parent(popover, session->terminal);

    g_signal_connect(entry, "search-changed", G_CALLBACK(search_changed), session);
    g_signal_connect(previous, "clicked", G_CALLBACK(search_previous_clicked), session);
    g_signal_connect(next, "clicked", G_CALLBACK(search_next_clicked), session);
    g_signal_connect(close, "clicked", G_CALLBACK(search_close_clicked), session);
    return popover;
}

static void
show_search(TerminalSessionView *session)
{
    if (session == NULL)
        return;

    if (session->search_popover == NULL)
        session->search_popover = build_search_popover(session);

    gtk_popover_popup(GTK_POPOVER(session->search_popover));
    gtk_widget_grab_focus(session->search_entry);
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
    begin_guarded_paste(GTK_WIDGET(user_data));
}

static void
terminal_action_find(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    show_search(session_view_for(GTK_WIDGET(user_data)));
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
    {"find", terminal_action_find, NULL, NULL, NULL, {0, 0, 0}},
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
    g_menu_append(edit, "Find", "terminal.find");
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

static void
tab_title_editing_changed(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    TerminalTabView *tab = user_data;

    (void) pspec;
    if (gtk_editable_label_get_editing(GTK_EDITABLE_LABEL(object)))
        return;

    const char *text = gtk_editable_get_text(GTK_EDITABLE(object));
    char *normalized = g_strdup(text != NULL ? text : "");
    g_strstrip(normalized);

    g_clear_pointer(&tab->custom_title, g_free);
    if (*normalized != '\0')
        tab->custom_title = g_strdup(normalized);
    g_free(normalized);
    update_tab_presentation(tab);
}

static void
tab_action_rename(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalTabView *tab = user_data;
    (void) action;
    (void) parameter;
    gtk_editable_label_start_editing(GTK_EDITABLE_LABEL(tab->tab_text));
}

static void
tab_action_reset_name(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalTabView *tab = user_data;
    (void) action;
    (void) parameter;
    g_clear_pointer(&tab->custom_title, g_free);
    update_tab_presentation(tab);
}

static void
tab_action_close(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    (void) action;
    (void) parameter;
    close_tab_view(user_data);
}

static const GActionEntry tab_actions[] = {
    {"rename", tab_action_rename, NULL, NULL, NULL, {0, 0, 0}},
    {"reset-name", tab_action_reset_name, NULL, NULL, NULL, {0, 0, 0}},
    {"close", tab_action_close, NULL, NULL, NULL, {0, 0, 0}},
};

static GtkWidget *
build_tab_context_menu(TerminalTabView *tab)
{
    GSimpleActionGroup *actions = g_simple_action_group_new();
    g_action_map_add_action_entries(
        G_ACTION_MAP(actions),
        tab_actions,
        G_N_ELEMENTS(tab_actions),
        tab);
    gtk_widget_insert_action_group(tab->tab_root, "tab", G_ACTION_GROUP(actions));
    g_object_unref(actions);

    GMenu *menu = g_menu_new();
    g_menu_append(menu, "Rename Tab", "tab.rename");
    g_menu_append(menu, "Reset Tab Name", "tab.reset-name");
    g_menu_append(menu, "Close Tab", "tab.close");

    GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
    gtk_widget_add_css_class(popover, "glaze-context-menu");
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
    gtk_widget_set_parent(popover, tab->tab_root);
    g_object_unref(menu);
    return popover;
}

static void
tab_menu_pressed(GtkGestureClick *gesture,
                 int n_press,
                 double x,
                 double y,
                 gpointer user_data)
{
    TerminalTabView *tab = user_data;
    GdkRectangle pointing_to = {(int) x, (int) y, 1, 1};

    (void) n_press;
    gtk_popover_set_pointing_to(GTK_POPOVER(tab->tab_menu), &pointing_to);
    gtk_popover_popup(GTK_POPOVER(tab->tab_menu));
    gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
tab_primary_pressed(GtkGestureClick *gesture,
                    int n_press,
                    double x,
                    double y,
                    gpointer user_data)
{
    TerminalTabView *tab = user_data;

    (void) x;
    (void) y;
    if (n_press == 2) {
        gtk_editable_label_start_editing(GTK_EDITABLE_LABEL(tab->tab_text));
        gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
    }
}

static void
close_tab_clicked(GtkButton *button, gpointer user_data)
{
    (void) button;
    close_tab_view(user_data);
}

static GtkWidget *
create_tab_label(TerminalTabView *tab)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *label = gtk_editable_label_new("");
    GtkWidget *close = gtk_button_new_from_icon_name("window-close-symbolic");

    tab->tab_root = box;
    tab->tab_text = label;
    gtk_widget_add_css_class(box, "glaze-tab-label");
    gtk_widget_add_css_class(close, "glaze-tab-close");
    gtk_button_set_has_frame(GTK_BUTTON(close), FALSE);
    gtk_widget_set_tooltip_text(close, "Close terminal tab");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(close),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Close terminal tab",
        -1);

    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), close);
    g_signal_connect(close, "clicked", G_CALLBACK(close_tab_clicked), tab);
    g_signal_connect(
        label,
        "notify::editing",
        G_CALLBACK(tab_title_editing_changed),
        tab);

    tab->tab_menu = build_tab_context_menu(tab);

    GtkGesture *tab_context_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(tab_context_click), GDK_BUTTON_SECONDARY);
    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(tab_context_click),
        GTK_PHASE_CAPTURE);
    g_signal_connect(
        tab_context_click,
        "pressed",
        G_CALLBACK(tab_menu_pressed),
        tab);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(tab_context_click));

    GtkGesture *tab_primary_click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(tab_primary_click), GDK_BUTTON_PRIMARY);
    g_signal_connect(
        tab_primary_click,
        "pressed",
        G_CALLBACK(tab_primary_pressed),
        tab);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(tab_primary_click));

    update_tab_presentation(tab);
    return box;
}

static TerminalTabView *
tab_view_new(TerminalWindow *terminal_window, const char *title)
{
    TerminalTabView *tab = g_new0(TerminalTabView, 1);
    tab->owner = terminal_window;
    tab->sessions = g_ptr_array_new();
    tab->default_title = g_strdup(title != NULL && *title != '\0' ? title : "Terminal");
    return tab;
}

static GtkWidget *
create_session_pane(TerminalWindow *terminal_window,
                    TerminalTabView *tab,
                    const GoreeTerminalSessionProfile *profile)
{
    guint session_id = terminal_window->next_session_id++;
    GtkWidget *pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *terminal = vte_terminal_new();
    TerminalSessionView *session = g_new0(TerminalSessionView, 1);

    goree_terminal_session_lifecycle_init(&session->lifecycle, session_id);
    goree_terminal_host_session_init(&session->host_session);
    session->terminal = terminal;
    session->pane_root = pane;
    session->owner = terminal_window;
    session->tab = tab;
    session->profile = profile;
    session->origin = running_in_flatpak()
        ? TERMINAL_SESSION_HOST_BRIDGE
        : TERMINAL_SESSION_LOCAL_HOST;

    g_object_set_data_full(
        G_OBJECT(terminal),
        SESSION_STATE_KEY,
        session,
        session_view_free);
    g_ptr_array_add(tab->sessions, session);
    if (tab->active_session == NULL)
        tab->active_session = session;

    gtk_widget_add_css_class(pane, "glaze-terminal-pane");
    gtk_widget_set_hexpand(pane, TRUE);
    gtk_widget_set_vexpand(pane, TRUE);
    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);
    gtk_box_append(GTK_BOX(pane), terminal);

    apply_terminal_preferences(terminal_window, VTE_TERMINAL(terminal), profile);
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

    GtkEventController *focus = gtk_event_controller_focus_new();
    g_signal_connect(focus, "enter", G_CALLBACK(session_focus_entered), session);
    gtk_widget_add_controller(terminal, focus);

    g_signal_connect(
        terminal,
        "child-exited",
        G_CALLBACK(session_child_exited),
        session);

    if (running_in_flatpak()) {
        if (!start_host_bridge_session(session))
            mark_host_session_unavailable(session);
        else
            update_session_presentation(terminal, session);
    } else if (goree_terminal_session_mark_running(&session->lifecycle)) {
        spawn_profile_shell(session);
        update_session_presentation(terminal, session);
    }

    if (tab->active_session == session)
        gtk_widget_add_css_class(pane, "glaze-active-pane");
    return pane;
}

static GtkWidget *
build_workspace_panes(TerminalWindow *terminal_window,
                      TerminalTabView *tab,
                      const GoreeTerminalWorkspaceTab *workspace_tab,
                      guint pane_index)
{
    const char *profile_id = g_ptr_array_index(
        workspace_tab->profile_ids,
        pane_index);
    const GoreeTerminalSessionProfile *profile = goree_terminal_runtime_catalog_find_profile(
        &terminal_window->catalog,
        profile_id);
    g_return_val_if_fail(profile != NULL, NULL);

    GtkWidget *pane = create_session_pane(terminal_window, tab, profile);
    if (pane_index + 1 >= workspace_tab->profile_ids->len)
        return pane;

    GtkOrientation orientation = workspace_tab->orientation == GOREE_TERMINAL_SPLIT_VERTICAL
        ? GTK_ORIENTATION_VERTICAL
        : GTK_ORIENTATION_HORIZONTAL;
    GtkWidget *split = gtk_paned_new(orientation);
    GtkWidget *remainder = build_workspace_panes(
        terminal_window,
        tab,
        workspace_tab,
        pane_index + 1);

    if (remainder == NULL)
        return pane;

    gtk_widget_add_css_class(split, "glaze-workspace-split");
    gtk_widget_set_hexpand(split, TRUE);
    gtk_widget_set_vexpand(split, TRUE);
    gtk_paned_set_start_child(GTK_PANED(split), pane);
    gtk_paned_set_end_child(GTK_PANED(split), remainder);
    gtk_paned_set_resize_start_child(GTK_PANED(split), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(split), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(split), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(split), FALSE);
    return split;
}

static void
append_workspace_tab(TerminalWindow *terminal_window,
                     const GoreeTerminalWorkspaceTab *workspace_tab)
{
    TerminalTabView *tab = tab_view_new(terminal_window, workspace_tab->title);
    GtkWidget *page_root = build_workspace_panes(
        terminal_window,
        tab,
        workspace_tab,
        0);
    if (page_root == NULL) {
        tab_view_free(tab);
        return;
    }

    tab->page_root = page_root;
    g_object_set_data_full(G_OBJECT(page_root), TAB_STATE_KEY, tab, tab_view_free);
    GtkWidget *tab_label = create_tab_label(tab);
    int page = gtk_notebook_append_page(
        GTK_NOTEBOOK(terminal_window->notebook),
        page_root,
        tab_label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal_window->notebook), page);

    GtkWidget *terminal = tab->active_session != NULL
        ? tab->active_session->terminal
        : first_terminal_in_widget(page_root);
    if (terminal != NULL)
        gtk_widget_grab_focus(terminal);
    update_tab_presentation(tab);
}

static void
add_profile_session(TerminalWindow *terminal_window,
                    const GoreeTerminalSessionProfile *profile)
{
    if (profile == NULL)
        return;

    GoreeTerminalWorkspaceTab tab_spec = {
        .title = profile->name,
        .orientation = GOREE_TERMINAL_SPLIT_HORIZONTAL,
        .profile_ids = g_ptr_array_new(),
    };
    g_ptr_array_add(tab_spec.profile_ids, profile->id);
    append_workspace_tab(terminal_window, &tab_spec);
    g_ptr_array_unref(tab_spec.profile_ids);
    update_open_tabs_menu(terminal_window);
}

static void
add_workspace(TerminalWindow *terminal_window,
              const GoreeTerminalWorkspace *workspace)
{
    if (workspace == NULL)
        return;

    for (guint index = 0; index < workspace->tabs->len; index++) {
        GoreeTerminalWorkspaceTab *workspace_tab = g_ptr_array_index(
            workspace->tabs,
            index);
        append_workspace_tab(terminal_window, workspace_tab);
    }
    update_open_tabs_menu(terminal_window);
}

static const GoreeTerminalSessionProfile *
default_profile(TerminalWindow *terminal_window)
{
    const GoreeTerminalSessionProfile *profile = goree_terminal_runtime_catalog_find_profile(
        &terminal_window->catalog,
        "default");
    if (profile == NULL && terminal_window->catalog.profiles != NULL &&
        terminal_window->catalog.profiles->len > 0)
        profile = g_ptr_array_index(terminal_window->catalog.profiles, 0);
    return profile;
}

static void
add_session(TerminalWindow *terminal_window)
{
    if (!terminal_window->catalog_ready)
        return;
    add_profile_session(terminal_window, default_profile(terminal_window));
}

static GtkWidget *
current_terminal(TerminalWindow *terminal_window)
{
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal_window->notebook));
    if (page < 0)
        return NULL;

    GtkWidget *page_root = gtk_notebook_get_nth_page(
        GTK_NOTEBOOK(terminal_window->notebook),
        page);
    TerminalTabView *tab = page_root != NULL
        ? g_object_get_data(G_OBJECT(page_root), TAB_STATE_KEY)
        : NULL;
    if (tab != NULL && tab->active_session != NULL)
        return tab->active_session->terminal;
    return first_terminal_in_widget(page_root);
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
action_new_profile(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    const char *profile_id = g_variant_get_string(parameter, NULL);
    const GoreeTerminalSessionProfile *profile = goree_terminal_runtime_catalog_find_profile(
        &terminal_window->catalog,
        profile_id);

    (void) action;
    add_profile_session(terminal_window, profile);
}

static void
action_open_workspace(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    const char *workspace_id = g_variant_get_string(parameter, NULL);
    const GoreeTerminalWorkspace *workspace = goree_terminal_runtime_catalog_find_workspace(
        &terminal_window->catalog,
        workspace_id);

    (void) action;
    add_workspace(terminal_window, workspace);
}

static void
action_close_session(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;

    (void) action;
    (void) parameter;
    GtkWidget *terminal = current_terminal(terminal_window);
    if (terminal != NULL)
        close_terminal_widget(terminal);
}

static void
action_find(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    (void) action;
    (void) parameter;

    GtkWidget *terminal = current_terminal(terminal_window);
    if (terminal != NULL)
        show_search(session_view_for(terminal));
}

static void
action_find_next(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    (void) action;
    (void) parameter;

    GtkWidget *terminal = current_terminal(terminal_window);
    if (terminal != NULL)
        vte_terminal_search_find_next(VTE_TERMINAL(terminal));
}

static void
action_find_previous(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    (void) action;
    (void) parameter;

    GtkWidget *terminal = current_terminal(terminal_window);
    if (terminal != NULL)
        vte_terminal_search_find_previous(VTE_TERMINAL(terminal));
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

static void
action_activate_tab(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    int page = g_variant_get_int32(parameter);

    (void) action;
    if (page >= 0 && page < gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal_window->notebook))) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal_window->notebook), page);
        GtkWidget *page_root = gtk_notebook_get_nth_page(
            GTK_NOTEBOOK(terminal_window->notebook),
            page);
        TerminalTabView *tab = page_root != NULL
            ? g_object_get_data(G_OBJECT(page_root), TAB_STATE_KEY)
            : NULL;
        GtkWidget *terminal = tab != NULL && tab->active_session != NULL
            ? tab->active_session->terminal
            : first_terminal_in_widget(page_root);
        if (terminal != NULL)
            gtk_widget_grab_focus(terminal);
    }
}

static void
action_show_open_tabs(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;

    (void) action;
    (void) parameter;
    update_open_tabs_menu(terminal_window);
    gtk_menu_button_popup(GTK_MENU_BUTTON(terminal_window->open_tabs_button));
}

static void
action_new_window(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;
    GtkApplication *application = gtk_window_get_application(GTK_WINDOW(terminal_window->window));

    (void) action;
    (void) parameter;
    if (application == NULL)
        return;

    TerminalWindow *new_window = create_terminal_window(application);
    gtk_window_present(GTK_WINDOW(new_window->window));
}

static void
action_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    TerminalWindow *terminal_window = user_data;

    (void) action;
    (void) parameter;
    gtk_show_about_dialog(
        GTK_WINDOW(terminal_window->window),
        "program-name", "GoreeCloud Terminal",
        "version", "0.1.0-dev",
        "comments", "Native GoreeCloud terminal built with GTK, VTE, and Glaze UI.",
        NULL);
}

static const GActionEntry window_actions[] = {
    {"new-session", action_new_session, NULL, NULL, NULL, {0, 0, 0}},
    {"new-profile", action_new_profile, "s", NULL, NULL, {0, 0, 0}},
    {"open-workspace", action_open_workspace, "s", NULL, NULL, {0, 0, 0}},
    {"close-session", action_close_session, NULL, NULL, NULL, {0, 0, 0}},
    {"find", action_find, NULL, NULL, NULL, {0, 0, 0}},
    {"find-next", action_find_next, NULL, NULL, NULL, {0, 0, 0}},
    {"find-previous", action_find_previous, NULL, NULL, NULL, {0, 0, 0}},
    {"set-theme", action_set_theme, "s", NULL, NULL, {0, 0, 0}},
    {"activate-tab", action_activate_tab, "i", NULL, NULL, {0, 0, 0}},
    {"show-open-tabs", action_show_open_tabs, NULL, NULL, NULL, {0, 0, 0}},
    {"new-window", action_new_window, NULL, NULL, NULL, {0, 0, 0}},
    {"about", action_about, NULL, NULL, NULL, {0, 0, 0}},
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

static GMenuModel *
build_profiles_menu(TerminalWindow *terminal_window)
{
    GMenu *menu = g_menu_new();
    if (!terminal_window->catalog_ready)
        return G_MENU_MODEL(menu);

    for (guint index = 0; index < terminal_window->catalog.profiles->len; index++) {
        GoreeTerminalSessionProfile *profile = g_ptr_array_index(
            terminal_window->catalog.profiles,
            index);
        GMenuItem *item = g_menu_item_new(profile->name, NULL);
        g_menu_item_set_action_and_target(
            item,
            "win.new-profile",
            "s",
            profile->id);
        g_menu_append_item(menu, item);
        g_object_unref(item);
    }
    return G_MENU_MODEL(menu);
}

static GMenuModel *
build_workspaces_menu(TerminalWindow *terminal_window)
{
    GMenu *menu = g_menu_new();
    if (!terminal_window->catalog_ready)
        return G_MENU_MODEL(menu);

    for (guint index = 0; index < terminal_window->catalog.workspaces->len; index++) {
        GoreeTerminalWorkspace *workspace = g_ptr_array_index(
            terminal_window->catalog.workspaces,
            index);
        GMenuItem *item = g_menu_item_new(workspace->name, NULL);
        g_menu_item_set_action_and_target(
            item,
            "win.open-workspace",
            "s",
            workspace->id);
        g_menu_append_item(menu, item);
        g_object_unref(item);
    }
    return G_MENU_MODEL(menu);
}

static GMenuModel *
build_main_menu(TerminalWindow *terminal_window)
{
    GMenu *menu = g_menu_new();
    GMenu *session = g_menu_new();
    GMenu *application = g_menu_new();
    GMenuModel *theme_menu = build_theme_menu();
    GMenuModel *profiles_menu = build_profiles_menu(terminal_window);
    GMenuModel *workspaces_menu = build_workspaces_menu(terminal_window);

    g_menu_append(session, "New Tab", "win.new-session");
    g_menu_append_submenu(session, "New Tab with Profile", profiles_menu);
    g_menu_append_submenu(session, "Open Workspace", workspaces_menu);
    g_menu_append(session, "New Window", "win.new-window");
    g_menu_append(session, "Show Open Tabs", "win.show-open-tabs");
    g_menu_append(session, "Find", "win.find");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(session));

    g_menu_append_submenu(application, "Theme", theme_menu);
    g_menu_append(application, "About GoreeCloud Terminal", "win.about");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(application));

    g_object_unref(theme_menu);
    g_object_unref(profiles_menu);
    g_object_unref(workspaces_menu);
    g_object_unref(session);
    g_object_unref(application);
    return G_MENU_MODEL(menu);
}

static void
update_open_tabs_menu(TerminalWindow *terminal_window)
{
    if (terminal_window == NULL || terminal_window->open_tabs_button == NULL)
        return;

    GMenu *menu = g_menu_new();
    int pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal_window->notebook));

    for (int page = 0; page < pages; page++) {
        GtkWidget *page_root = gtk_notebook_get_nth_page(
            GTK_NOTEBOOK(terminal_window->notebook),
            page);
        TerminalTabView *tab = page_root != NULL
            ? g_object_get_data(G_OBJECT(page_root), TAB_STATE_KEY)
            : NULL;
        const char *label = NULL;

        if (tab != NULL && tab->tab_text != NULL)
            label = gtk_editable_get_text(GTK_EDITABLE(tab->tab_text));
        if (label == NULL || *label == '\0')
            label = "Terminal";

        GMenuItem *item = g_menu_item_new(label, NULL);
        g_menu_item_set_action_and_target(item, "win.activate-tab", "i", page);
        g_menu_append_item(menu, item);
        g_object_unref(item);
    }

    GMenu *commands = g_menu_new();
    g_menu_append(commands, "New Tab", "win.new-session");
    g_menu_append_section(menu, NULL, G_MENU_MODEL(commands));
    g_object_unref(commands);

    gtk_menu_button_set_menu_model(
        GTK_MENU_BUTTON(terminal_window->open_tabs_button),
        G_MENU_MODEL(menu));
    g_object_unref(menu);

    GtkPopover *popover = gtk_menu_button_get_popover(
        GTK_MENU_BUTTON(terminal_window->open_tabs_button));
    if (popover != NULL)
        gtk_widget_add_css_class(GTK_WIDGET(popover), "glaze-context-menu");
}

static void
add_catalog_error_page(TerminalWindow *terminal_window)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *title = gtk_label_new("Terminal profiles are unavailable");
    GtkWidget *detail = gtk_label_new(
        "GoreeCloud Terminal did not start a shell because the local profile/workspace catalog could not be validated. Correct the private configuration and reopen the window.");

    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_start(box, 32);
    gtk_widget_set_margin_end(box, 32);
    gtk_label_set_wrap(GTK_LABEL(detail), TRUE);
    gtk_widget_add_css_class(title, "title");
    gtk_box_append(GTK_BOX(box), title);
    gtk_box_append(GTK_BOX(box), detail);
    gtk_notebook_append_page(
        GTK_NOTEBOOK(terminal_window->notebook),
        box,
        gtk_label_new("Configuration"));
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

    goree_terminal_runtime_catalog_clear(&terminal_window->catalog);
    g_free(terminal_window->catalog_error);
    g_free(terminal_window);
}

static TerminalWindow *
create_terminal_window(GtkApplication *application)
{
    TerminalWindow *terminal_window = g_new0(TerminalWindow, 1);
    GError *preferences_error = NULL;
    GError *catalog_error = NULL;

    goree_terminal_theme_engine_init(&terminal_window->theme_engine);
    goree_terminal_preferences_init(&terminal_window->preferences);
    if (!goree_terminal_preferences_load(
            &terminal_window->preferences,
            &preferences_error)) {
        g_warning(
            "Unable to load GoreeCloud Terminal preferences; using safe defaults: %s",
            preferences_error != NULL ? preferences_error->message : "unknown error");
        g_clear_error(&preferences_error);
        goree_terminal_preferences_init(&terminal_window->preferences);
    }

    terminal_window->catalog_ready = goree_terminal_runtime_catalog_load(
        &terminal_window->catalog,
        &catalog_error);
    if (!terminal_window->catalog_ready) {
        terminal_window->catalog_error = g_strdup(
            catalog_error != NULL ? catalog_error->message : "unknown catalog error");
        g_warning(
            "Unable to load GoreeCloud Terminal profile/workspace catalog; refusing to start a shell: %s",
            terminal_window->catalog_error);
        g_clear_error(&catalog_error);
    }

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
    GtkWidget *open_tabs_button = gtk_menu_button_new();
    GtkWidget *menu_button = gtk_menu_button_new();
    GtkWidget *notebook = gtk_notebook_new();

    terminal_window->window = window;
    terminal_window->notebook = notebook;
    terminal_window->theme_button = theme_button;
    terminal_window->open_tabs_button = open_tabs_button;
    terminal_window->menu_button = menu_button;

    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
    gtk_widget_set_size_request(window, 360, 240);
    gtk_widget_add_css_class(window, "glaze-window");
    gtk_widget_add_css_class(header, "glaze-header");
    gtk_widget_add_css_class(title, "title");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title);
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header), TRUE);

    gtk_widget_add_css_class(new_session, "glaze-action");
    gtk_button_set_has_frame(GTK_BUTTON(new_session), FALSE);
    gtk_widget_set_sensitive(new_session, terminal_window->catalog_ready);
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

    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(open_tabs_button), "view-grid-symbolic");
    gtk_widget_add_css_class(open_tabs_button, "glaze-icon-menu");
    gtk_widget_set_tooltip_text(open_tabs_button, "Open Tabs (Ctrl+Shift+O)");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(open_tabs_button),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Open Tabs",
        -1);

    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_widget_add_css_class(menu_button, "glaze-icon-menu");
    gtk_widget_set_tooltip_text(menu_button, "Menu");
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(menu_button),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        "Menu",
        -1);
    GMenuModel *main_menu = build_main_menu(terminal_window);
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), main_menu);
    g_object_unref(main_menu);
    GtkPopover *main_popover = gtk_menu_button_get_popover(GTK_MENU_BUTTON(menu_button));
    if (main_popover != NULL)
        gtk_widget_add_css_class(GTK_WIDGET(main_popover), "glaze-context-menu");

    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), menu_button);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), open_tabs_button);
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
    const char *new_window_accels[] = {"<Primary><Shift>n", NULL};
    const char *open_tabs_accels[] = {"<Primary><Shift>o", NULL};
    const char *find_accels[] = {"<Primary><Shift>f", NULL};
    const char *find_next_accels[] = {"F3", NULL};
    const char *find_previous_accels[] = {"<Shift>F3", NULL};
    gtk_application_set_accels_for_action(application, "win.new-session", new_session_accels);
    gtk_application_set_accels_for_action(application, "win.close-session", close_session_accels);
    gtk_application_set_accels_for_action(application, "win.new-window", new_window_accels);
    gtk_application_set_accels_for_action(application, "win.show-open-tabs", open_tabs_accels);
    gtk_application_set_accels_for_action(application, "win.find", find_accels);
    gtk_application_set_accels_for_action(application, "win.find-next", find_next_accels);
    gtk_application_set_accels_for_action(application, "win.find-previous", find_previous_accels);

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
    if (terminal_window->catalog_ready) {
        const GoreeTerminalWorkspace *default_workspace = goree_terminal_runtime_catalog_find_workspace(
            &terminal_window->catalog,
            "default");
        if (default_workspace != NULL)
            add_workspace(terminal_window, default_workspace);
        else
            add_session(terminal_window);
    } else {
        add_catalog_error_page(terminal_window);
    }
    update_open_tabs_menu(terminal_window);
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
