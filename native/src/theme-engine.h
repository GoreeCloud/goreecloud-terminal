#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
    GOREE_TERMINAL_THEME_FOLLOW_SYSTEM = 0,
    GOREE_TERMINAL_THEME_LIGHT,
    GOREE_TERMINAL_THEME_DARK,
    GOREE_TERMINAL_THEME_DEEP_DARK,
    GOREE_TERMINAL_THEME_COUNT
} GoreeTerminalTheme;

typedef struct {
    const char *id;
    const char *label;
    gboolean follows_system;
    gboolean prefers_dark;
    const char *foreground;
    const char *background;
    const char *cursor;
    const char *selection_background;
} GoreeTerminalThemeDefinition;

typedef struct {
    GoreeTerminalTheme active;
} GoreeTerminalThemeEngine;

/*
 * The Theme Engine owns the terminal-specific appearance preference. The
 * persisted file contains only the selected theme ID and never terminal
 * contents, commands, paths, hosts, credentials, or other session data.
 *
 * GOREE_TERMINAL_THEME_SETTINGS_PATH may override the XDG path for isolated
 * testing. Production defaults to $XDG_CONFIG_HOME/goreecloud/terminal/theme.ini.
 */
void goree_terminal_theme_engine_init(GoreeTerminalThemeEngine *engine);
gboolean goree_terminal_theme_engine_set(GoreeTerminalThemeEngine *engine, const char *id);
GoreeTerminalTheme goree_terminal_theme_engine_get(const GoreeTerminalThemeEngine *engine);
const GoreeTerminalThemeDefinition *goree_terminal_theme_definition(GoreeTerminalTheme theme);
const GoreeTerminalThemeDefinition *goree_terminal_theme_resolve(GoreeTerminalTheme theme,
                                                                  gboolean system_prefers_dark);
const char *goree_terminal_theme_id(GoreeTerminalTheme theme);
const char *goree_terminal_theme_label(GoreeTerminalTheme theme);
char *goree_terminal_theme_settings_path(void);

G_END_DECLS
