#include "theme-engine.h"

#include <errno.h>
#include <glib/gstdio.h>

#define THEME_SETTINGS_ENV "GOREE_TERMINAL_THEME_SETTINGS_PATH"
#define THEME_SETTINGS_GROUP "Theme"
#define THEME_SETTINGS_KEY "mode"

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

static gboolean
theme_for_id(const char *id, GoreeTerminalTheme *theme_out)
{
    g_return_val_if_fail(id != NULL, FALSE);

    for (guint i = 0; i < GOREE_TERMINAL_THEME_COUNT; i++) {
        if (g_strcmp0(themes[i].id, id) == 0) {
            if (theme_out != NULL)
                *theme_out = (GoreeTerminalTheme) i;
            return TRUE;
        }
    }

    return FALSE;
}

char *
goree_terminal_theme_settings_path(void)
{
    const char *override = g_getenv(THEME_SETTINGS_ENV);

    if (override != NULL && *override != '\0')
        return g_strdup(override);

    return g_build_filename(
        g_get_user_config_dir(),
        "goreecloud",
        "terminal",
        "theme.ini",
        NULL);
}

static void
load_persisted_theme(GoreeTerminalThemeEngine *engine)
{
    char *path = goree_terminal_theme_settings_path();

    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_free(path);
        return;
    }

    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;

    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error)) {
        g_warning("Could not read the persisted GoreeCloud Terminal theme preference; using Follow System.");
        g_clear_error(&error);
        g_key_file_unref(key_file);
        g_free(path);
        return;
    }

    char *mode = g_key_file_get_string(
        key_file,
        THEME_SETTINGS_GROUP,
        THEME_SETTINGS_KEY,
        &error);
    GoreeTerminalTheme persisted = GOREE_TERMINAL_THEME_FOLLOW_SYSTEM;

    if (mode != NULL && theme_for_id(mode, &persisted)) {
        engine->active = persisted;
    } else {
        g_warning("The persisted GoreeCloud Terminal theme preference is invalid; using Follow System.");
        engine->active = GOREE_TERMINAL_THEME_FOLLOW_SYSTEM;
    }

    g_clear_error(&error);
    g_free(mode);
    g_key_file_unref(key_file);
    g_free(path);
}

static gboolean
persist_theme(GoreeTerminalTheme theme)
{
    char *path = goree_terminal_theme_settings_path();
    char *directory = g_path_get_dirname(path);
    gboolean success = FALSE;

    if (g_mkdir_with_parents(directory, 0700) != 0) {
        g_warning("Could not create GoreeCloud Terminal's private theme-preference directory.");
        goto out;
    }

    GKeyFile *key_file = g_key_file_new();
    g_key_file_set_string(
        key_file,
        THEME_SETTINGS_GROUP,
        THEME_SETTINGS_KEY,
        goree_terminal_theme_id(theme));

    gsize length = 0;
    char *data = g_key_file_to_data(key_file, &length, NULL);
    GError *error = NULL;

    if (!g_file_set_contents(path, data, (gssize) length, &error)) {
        g_warning("Could not persist the GoreeCloud Terminal theme preference.");
        g_clear_error(&error);
    } else {
        if (g_chmod(path, 0600) != 0)
            g_warning("Could not restrict GoreeCloud Terminal theme-preference file permissions.");
        success = TRUE;
    }

    g_free(data);
    g_key_file_unref(key_file);

out:
    g_free(directory);
    g_free(path);
    return success;
}

void
goree_terminal_theme_engine_init(GoreeTerminalThemeEngine *engine)
{
    g_return_if_fail(engine != NULL);

    engine->active = GOREE_TERMINAL_THEME_FOLLOW_SYSTEM;
    load_persisted_theme(engine);
}

gboolean
goree_terminal_theme_engine_set(GoreeTerminalThemeEngine *engine, const char *id)
{
    GoreeTerminalTheme selected;

    g_return_val_if_fail(engine != NULL, FALSE);
    g_return_val_if_fail(id != NULL, FALSE);

    if (!theme_for_id(id, &selected))
        return FALSE;

    engine->active = selected;
    persist_theme(selected);
    return TRUE;
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
