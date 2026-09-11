#include "theme-engine.h"

static const GoreeTerminalThemeDefinition themes[GOREE_TERMINAL_THEME_COUNT] = {
    [GOREE_TERMINAL_THEME_FOLLOW_SYSTEM] = {
        .id = "follow-system",
        .label = "System",
        .follows_system = TRUE,
        .prefers_dark = FALSE,
        .foreground = NULL,
        .background = NULL,
        .cursor = "#68AEE0",
        .selection_background = "#68AEE0",
    },
    [GOREE_TERMINAL_THEME_LIGHT] = {
        .id = "light",
        .label = "Light",
        .follows_system = FALSE,
        .prefers_dark = FALSE,
        .foreground = "#1B1D22",
        .background = "#F7F8FA",
        .cursor = "#2B6FA3",
        .selection_background = "#B9DCF2",
    },
    [GOREE_TERMINAL_THEME_DARK] = {
        .id = "dark",
        .label = "Dark",
        .follows_system = FALSE,
        .prefers_dark = TRUE,
        .foreground = "#F3F5F7",
        .background = "#15171B",
        .cursor = "#68AEE0",
        .selection_background = "#31566F",
    },
    [GOREE_TERMINAL_THEME_DEEP_DARK] = {
        .id = "deep-dark",
        .label = "Deep Dark",
        .follows_system = FALSE,
        .prefers_dark = TRUE,
        .foreground = "#F7F8FA",
        .background = "#090A0C",
        .cursor = "#68AEE0",
        .selection_background = "#24485F",
    },
};

void
goree_terminal_theme_engine_init(GoreeTerminalThemeEngine *engine)
{
    g_return_if_fail(engine != NULL);
    engine->active = GOREE_TERMINAL_THEME_FOLLOW_SYSTEM;
}

gboolean
goree_terminal_theme_engine_set(GoreeTerminalThemeEngine *engine, const char *id)
{
    g_return_val_if_fail(engine != NULL, FALSE);
    g_return_val_if_fail(id != NULL, FALSE);

    for (guint i = 0; i < GOREE_TERMINAL_THEME_COUNT; i++) {
        if (g_strcmp0(themes[i].id, id) == 0) {
            engine->active = (GoreeTerminalTheme) i;
            return TRUE;
        }
    }

    return FALSE;
}

GoreeTerminalTheme
goree_terminal_theme_engine_get(const GoreeTerminalThemeEngine *engine)
{
    g_return_val_if_fail(engine != NULL, GOREE_TERMINAL_THEME_FOLLOW_SYSTEM);
    return engine->active;
}

const GoreeTerminalThemeDefinition *
goree_terminal_theme_definition(GoreeTerminalTheme theme)
{
    if (theme < 0 || theme >= GOREE_TERMINAL_THEME_COUNT)
        return &themes[GOREE_TERMINAL_THEME_FOLLOW_SYSTEM];

    return &themes[theme];
}

const GoreeTerminalThemeDefinition *
goree_terminal_theme_resolve(GoreeTerminalTheme theme, gboolean system_prefers_dark)
{
    if (theme == GOREE_TERMINAL_THEME_FOLLOW_SYSTEM)
        return system_prefers_dark ? &themes[GOREE_TERMINAL_THEME_DARK]
                                   : &themes[GOREE_TERMINAL_THEME_LIGHT];

    return goree_terminal_theme_definition(theme);
}

const char *
goree_terminal_theme_id(GoreeTerminalTheme theme)
{
    return goree_terminal_theme_definition(theme)->id;
}

const char *
goree_terminal_theme_label(GoreeTerminalTheme theme)
{
    return goree_terminal_theme_definition(theme)->label;
}
