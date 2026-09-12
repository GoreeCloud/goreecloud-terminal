#include "terminal-preferences.h"

#include <errno.h>
#include <glib/gstdio.h>

#define SETTINGS_GROUP "Terminal"
#define SETTINGS_FILENAME "preferences.ini"

static char *preferences_path = NULL;

static const char *
resolve_preferences_path(void)
{
    const char *override = g_getenv("GOREE_TERMINAL_PREFERENCES_PATH");

    if (override != NULL && *override != '\0')
        return override;

    if (preferences_path == NULL) {
        preferences_path = g_build_filename(
            g_get_user_config_dir(),
            "goreecloud",
            "terminal",
            SETTINGS_FILENAME,
            NULL);
    }
    return preferences_path;
}

const char *
goree_terminal_preferences_path(void)
{
    return resolve_preferences_path();
}

void
goree_terminal_preferences_init(GoreeTerminalPreferences *preferences)
{
    g_return_if_fail(preferences != NULL);

    preferences->scrollback_lines = GOREE_TERMINAL_SCROLLBACK_DEFAULT;
    preferences->audible_bell = FALSE;
    preferences->allow_hyperlinks = TRUE;
    preferences->search_wrap_around = TRUE;
    preferences->paste_protection = GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE;
}

const char *
goree_terminal_paste_protection_id(GoreeTerminalPasteProtection protection)
{
    switch (protection) {
    case GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS:
        return "always";
    case GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE:
    default:
        return "multiline";
    }
}

gboolean
goree_terminal_paste_protection_from_id(
    const char *id,
    GoreeTerminalPasteProtection *protection)
{
    g_return_val_if_fail(protection != NULL, FALSE);

    if (g_strcmp0(id, "multiline") == 0) {
        *protection = GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE;
        return TRUE;
    }
    if (g_strcmp0(id, "always") == 0) {
        *protection = GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS;
        return TRUE;
    }
    return FALSE;
}

static guint
load_scrollback(GKeyFile *key_file)
{
    GError *error = NULL;
    guint64 value = g_key_file_get_uint64(
        key_file,
        SETTINGS_GROUP,
        "scrollback-lines",
        &error);

    if (error != NULL) {
        g_clear_error(&error);
        return GOREE_TERMINAL_SCROLLBACK_DEFAULT;
    }
    if (value > GOREE_TERMINAL_SCROLLBACK_MAX)
        return GOREE_TERMINAL_SCROLLBACK_MAX;
    return (guint) value;
}

static gboolean
load_boolean_or_default(GKeyFile *key_file, const char *key, gboolean fallback)
{
    GError *error = NULL;
    gboolean value = g_key_file_get_boolean(key_file, SETTINGS_GROUP, key, &error);

    if (error != NULL) {
        g_clear_error(&error);
        return fallback;
    }
    return value;
}

gboolean
goree_terminal_preferences_load(
    GoreeTerminalPreferences *preferences,
    GError **error)
{
    GKeyFile *key_file;
    char *paste_id = NULL;
    GError *local_error = NULL;

    g_return_val_if_fail(preferences != NULL, FALSE);
    goree_terminal_preferences_init(preferences);

    key_file = g_key_file_new();
    if (!g_key_file_load_from_file(
            key_file,
            resolve_preferences_path(),
            G_KEY_FILE_NONE,
            &local_error)) {
        if (g_error_matches(local_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_clear_error(&local_error);
            g_key_file_unref(key_file);
            return TRUE;
        }
        g_propagate_error(error, local_error);
        g_key_file_unref(key_file);
        return FALSE;
    }

    preferences->scrollback_lines = load_scrollback(key_file);
    preferences->audible_bell = load_boolean_or_default(
        key_file,
        "audible-bell",
        FALSE);
    preferences->allow_hyperlinks = load_boolean_or_default(
        key_file,
        "allow-hyperlinks",
        TRUE);
    preferences->search_wrap_around = load_boolean_or_default(
        key_file,
        "search-wrap-around",
        TRUE);

    paste_id = g_key_file_get_string(
        key_file,
        SETTINGS_GROUP,
        "paste-protection",
        NULL);
    if (paste_id != NULL) {
        GoreeTerminalPasteProtection parsed;
        if (goree_terminal_paste_protection_from_id(paste_id, &parsed))
            preferences->paste_protection = parsed;
    }

    g_free(paste_id);
    g_key_file_unref(key_file);
    return TRUE;
}

gboolean
goree_terminal_preferences_save(
    const GoreeTerminalPreferences *preferences,
    GError **error)
{
    GKeyFile *key_file;
    char *data;
    gsize length;
    char *directory;
    const char *path;
    guint scrollback;
    gboolean saved;

    g_return_val_if_fail(preferences != NULL, FALSE);

    path = resolve_preferences_path();
    directory = g_path_get_dirname(path);
    if (g_mkdir_with_parents(directory, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(error,
                    G_FILE_ERROR,
                    g_file_error_from_errno(saved_errno),
                    "Unable to create preferences directory: %s",
                    g_strerror(saved_errno));
        g_free(directory);
        return FALSE;
    }
    g_free(directory);

    key_file = g_key_file_new();
    scrollback = MIN(preferences->scrollback_lines, GOREE_TERMINAL_SCROLLBACK_MAX);
    g_key_file_set_uint64(key_file, SETTINGS_GROUP, "scrollback-lines", scrollback);
    g_key_file_set_boolean(key_file, SETTINGS_GROUP, "audible-bell", preferences->audible_bell);
    g_key_file_set_boolean(key_file, SETTINGS_GROUP, "allow-hyperlinks", preferences->allow_hyperlinks);
    g_key_file_set_boolean(
        key_file,
        SETTINGS_GROUP,
        "search-wrap-around",
        preferences->search_wrap_around);
    g_key_file_set_string(
        key_file,
        SETTINGS_GROUP,
        "paste-protection",
        goree_terminal_paste_protection_id(preferences->paste_protection));

    data = g_key_file_to_data(key_file, &length, NULL);
    saved = g_file_set_contents(path, data, length, error);
    g_free(data);
    g_key_file_unref(key_file);

    if (saved && g_chmod(path, 0600) != 0) {
        int saved_errno = errno;
        g_set_error(error,
                    G_FILE_ERROR,
                    g_file_error_from_errno(saved_errno),
                    "Unable to secure preferences file: %s",
                    g_strerror(saved_errno));
        return FALSE;
    }

    return saved;
}
