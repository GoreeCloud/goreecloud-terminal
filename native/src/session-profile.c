#include "session-profile.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <string.h>

#define PROFILE_FILE_NAME "profiles.ini"
#define PROFILE_GROUP_PREFIX "Profile:"
#define PROFILE_ID_MAX 64
#define PROFILE_NAME_MAX 128

static char *profiles_path = NULL;

static const char *
resolve_profiles_path(void)
{
    const char *override = g_getenv("GOREE_TERMINAL_PROFILES_PATH");

    if (override != NULL && *override != '\0')
        return override;

    if (profiles_path == NULL) {
        profiles_path = g_build_filename(
            g_get_user_config_dir(),
            "goreecloud",
            "terminal",
            PROFILE_FILE_NAME,
            NULL);
    }
    return profiles_path;
}

const char *
goree_terminal_session_profiles_path(void)
{
    return resolve_profiles_path();
}

static gboolean
valid_profile_id(const char *id)
{
    if (id == NULL || *id == '\0' || strlen(id) > PROFILE_ID_MAX)
        return FALSE;

    for (const char *cursor = id; *cursor != '\0'; cursor++) {
        if (!(g_ascii_islower(*cursor) || g_ascii_isdigit(*cursor) ||
              *cursor == '-' || *cursor == '_'))
            return FALSE;
    }
    return TRUE;
}

static gboolean
valid_environment_name(const char *name)
{
    if (name == NULL || *name == '\0' ||
        !(g_ascii_isalpha(*name) || *name == '_'))
        return FALSE;

    for (const char *cursor = name + 1; *cursor != '\0'; cursor++) {
        if (!(g_ascii_isalnum(*cursor) || *cursor == '_'))
            return FALSE;
    }
    return TRUE;
}

static gboolean
valid_theme_id(const char *theme_id)
{
    return theme_id == NULL || *theme_id == '\0' ||
           g_str_equal(theme_id, "follow-system") ||
           g_str_equal(theme_id, "light") ||
           g_str_equal(theme_id, "dark") ||
           g_str_equal(theme_id, "deep-dark");
}

static void
set_validation_error(GError **error, const char *message)
{
    g_set_error_literal(
        error,
        G_OPTION_ERROR,
        G_OPTION_ERROR_BAD_VALUE,
        message);
}

GoreeTerminalSessionProfile *
goree_terminal_session_profile_new_default(void)
{
    GoreeTerminalSessionProfile *profile = g_new0(GoreeTerminalSessionProfile, 1);
    profile->id = g_strdup("default");
    profile->name = g_strdup("Default");
    profile->shell_path = g_strdup("");
    profile->working_directory = g_strdup("");
    profile->theme_id = g_strdup("follow-system");
    profile->scrollback_lines = GOREE_TERMINAL_SCROLLBACK_DEFAULT;
    profile->environment_policy = GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE;
    profile->environment_allowlist = g_new0(char *, 1);
    return profile;
}

GoreeTerminalSessionProfile *
goree_terminal_session_profile_copy(const GoreeTerminalSessionProfile *profile)
{
    g_return_val_if_fail(profile != NULL, NULL);

    GoreeTerminalSessionProfile *copy = g_new0(GoreeTerminalSessionProfile, 1);
    copy->id = g_strdup(profile->id);
    copy->name = g_strdup(profile->name);
    copy->shell_path = g_strdup(profile->shell_path);
    copy->working_directory = g_strdup(profile->working_directory);
    copy->theme_id = g_strdup(profile->theme_id);
    copy->scrollback_lines = profile->scrollback_lines;
    copy->environment_policy = profile->environment_policy;
    copy->environment_allowlist = g_strdupv(profile->environment_allowlist);
    return copy;
}

void
goree_terminal_session_profile_free(GoreeTerminalSessionProfile *profile)
{
    if (profile == NULL)
        return;

    g_free(profile->id);
    g_free(profile->name);
    g_free(profile->shell_path);
    g_free(profile->working_directory);
    g_free(profile->theme_id);
    g_strfreev(profile->environment_allowlist);
    g_free(profile);
}

const char *
goree_terminal_environment_policy_id(GoreeTerminalEnvironmentPolicy policy)
{
    switch (policy) {
    case GOREE_TERMINAL_ENVIRONMENT_CLEAN:
        return "clean";
    case GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE:
    default:
        return "inherit-safe";
    }
}

gboolean
goree_terminal_environment_policy_from_id(
    const char *id,
    GoreeTerminalEnvironmentPolicy *policy)
{
    g_return_val_if_fail(policy != NULL, FALSE);

    if (g_strcmp0(id, "inherit-safe") == 0) {
        *policy = GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE;
        return TRUE;
    }
    if (g_strcmp0(id, "clean") == 0) {
        *policy = GOREE_TERMINAL_ENVIRONMENT_CLEAN;
        return TRUE;
    }
    return FALSE;
}

gboolean
goree_terminal_session_profile_validate(
    const GoreeTerminalSessionProfile *profile,
    GError **error)
{
    g_return_val_if_fail(profile != NULL, FALSE);

    if (!valid_profile_id(profile->id)) {
        set_validation_error(error, "Profile ID must use lowercase letters, digits, '-' or '_'.");
        return FALSE;
    }
    if (profile->name == NULL || *profile->name == '\0' ||
        !g_utf8_validate(profile->name, -1, NULL) ||
        g_utf8_strlen(profile->name, -1) > PROFILE_NAME_MAX) {
        set_validation_error(error, "Profile name is missing, invalid, or too long.");
        return FALSE;
    }
    if (profile->shell_path != NULL && *profile->shell_path != '\0' &&
        (!g_path_is_absolute(profile->shell_path) ||
         !g_utf8_validate(profile->shell_path, -1, NULL))) {
        set_validation_error(error, "Profile shell path must be an absolute UTF-8 path.");
        return FALSE;
    }
    if (profile->working_directory != NULL && *profile->working_directory != '\0' &&
        (!g_path_is_absolute(profile->working_directory) ||
         !g_utf8_validate(profile->working_directory, -1, NULL))) {
        set_validation_error(error, "Profile working directory must be an absolute UTF-8 path.");
        return FALSE;
    }
    if (!valid_theme_id(profile->theme_id)) {
        set_validation_error(error, "Profile theme is not an approved GoreeCloud Terminal theme ID.");
        return FALSE;
    }
    if (profile->scrollback_lines > GOREE_TERMINAL_SCROLLBACK_MAX) {
        set_validation_error(error, "Profile scrollback exceeds the supported bound.");
        return FALSE;
    }
    if (profile->environment_policy != GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE &&
        profile->environment_policy != GOREE_TERMINAL_ENVIRONMENT_CLEAN) {
        set_validation_error(error, "Profile environment policy is invalid.");
        return FALSE;
    }

    if (profile->environment_allowlist != NULL) {
        for (char **name = profile->environment_allowlist; *name != NULL; name++) {
            if (!valid_environment_name(*name)) {
                set_validation_error(error, "Profile environment allowlist contains an invalid variable name.");
                return FALSE;
            }
            if (strchr(*name, '=') != NULL) {
                set_validation_error(error, "Profile environment allowlist must contain names only, never values.");
                return FALSE;
            }
        }
    }

    return TRUE;
}

static GoreeTerminalSessionProfile *
load_profile(GKeyFile *key_file, const char *group, GError **error)
{
    GoreeTerminalSessionProfile *profile = g_new0(GoreeTerminalSessionProfile, 1);
    const char *id = group + strlen(PROFILE_GROUP_PREFIX);
    char *environment_policy = NULL;
    gsize allowlist_length = 0;

    profile->id = g_strdup(id);
    profile->name = g_key_file_get_string(key_file, group, "name", error);
    if (profile->name == NULL)
        goto invalid;

    profile->shell_path = g_key_file_get_string(key_file, group, "shell", NULL);
    if (profile->shell_path == NULL)
        profile->shell_path = g_strdup("");

    profile->working_directory = g_key_file_get_string(
        key_file,
        group,
        "working-directory",
        NULL);
    if (profile->working_directory == NULL)
        profile->working_directory = g_strdup("");

    profile->theme_id = g_key_file_get_string(key_file, group, "theme", NULL);
    if (profile->theme_id == NULL)
        profile->theme_id = g_strdup("follow-system");

    GError *scrollback_error = NULL;
    guint64 scrollback = g_key_file_get_uint64(
        key_file,
        group,
        "scrollback-lines",
        &scrollback_error);
    if (scrollback_error != NULL) {
        g_clear_error(&scrollback_error);
        scrollback = GOREE_TERMINAL_SCROLLBACK_DEFAULT;
    }
    profile->scrollback_lines = (guint) MIN(
        scrollback,
        (guint64) GOREE_TERMINAL_SCROLLBACK_MAX);

    environment_policy = g_key_file_get_string(
        key_file,
        group,
        "environment-policy",
        NULL);
    if (environment_policy == NULL)
        environment_policy = g_strdup("inherit-safe");
    if (!goree_terminal_environment_policy_from_id(
            environment_policy,
            &profile->environment_policy)) {
        set_validation_error(error, "Profile environment policy is invalid.");
        goto invalid;
    }

    profile->environment_allowlist = g_key_file_get_string_list(
        key_file,
        group,
        "environment-allowlist",
        &allowlist_length,
        NULL);
    if (profile->environment_allowlist == NULL)
        profile->environment_allowlist = g_new0(char *, 1);

    g_free(environment_policy);
    if (!goree_terminal_session_profile_validate(profile, error))
        goto invalid_no_policy;
    return profile;

invalid:
    g_free(environment_policy);
invalid_no_policy:
    goree_terminal_session_profile_free(profile);
    return NULL;
}

GPtrArray *
goree_terminal_session_profiles_load(GError **error)
{
    GPtrArray *profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);
    GKeyFile *key_file = g_key_file_new();
    GError *local_error = NULL;

    if (!g_key_file_load_from_file(
            key_file,
            resolve_profiles_path(),
            G_KEY_FILE_NONE,
            &local_error)) {
        if (g_error_matches(local_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_clear_error(&local_error);
            g_key_file_unref(key_file);
            g_ptr_array_add(profiles, goree_terminal_session_profile_new_default());
            return profiles;
        }
        g_propagate_error(error, local_error);
        g_key_file_unref(key_file);
        g_ptr_array_unref(profiles);
        return NULL;
    }

    gsize group_count = 0;
    char **groups = g_key_file_get_groups(key_file, &group_count);
    for (gsize i = 0; i < group_count; i++) {
        if (!g_str_has_prefix(groups[i], PROFILE_GROUP_PREFIX))
            continue;

        GoreeTerminalSessionProfile *profile = load_profile(
            key_file,
            groups[i],
            error);
        if (profile == NULL) {
            g_strfreev(groups);
            g_key_file_unref(key_file);
            g_ptr_array_unref(profiles);
            return NULL;
        }
        g_ptr_array_add(profiles, profile);
    }

    g_strfreev(groups);
    g_key_file_unref(key_file);

    if (profiles->len == 0)
        g_ptr_array_add(profiles, goree_terminal_session_profile_new_default());
    return profiles;
}

static gboolean
ensure_parent_directory(const char *path, GError **error)
{
    char *directory = g_path_get_dirname(path);
    if (g_mkdir_with_parents(directory, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to create profile directory: %s",
            g_strerror(saved_errno));
        g_free(directory);
        return FALSE;
    }
    g_free(directory);
    return TRUE;
}

gboolean
goree_terminal_session_profiles_save(
    const GPtrArray *profiles,
    GError **error)
{
    g_return_val_if_fail(profiles != NULL, FALSE);

    if (profiles->len == 0) {
        set_validation_error(error, "At least one terminal profile is required.");
        return FALSE;
    }

    GHashTable *ids = g_hash_table_new(g_str_hash, g_str_equal);
    GKeyFile *key_file = g_key_file_new();

    for (guint i = 0; i < profiles->len; i++) {
        const GoreeTerminalSessionProfile *profile = g_ptr_array_index(profiles, i);
        if (!goree_terminal_session_profile_validate(profile, error))
            goto failure;
        if (g_hash_table_contains(ids, profile->id)) {
            set_validation_error(error, "Terminal profile IDs must be unique.");
            goto failure;
        }
        g_hash_table_add(ids, profile->id);

        char *group = g_strdup_printf("%s%s", PROFILE_GROUP_PREFIX, profile->id);
        g_key_file_set_string(key_file, group, "name", profile->name);
        g_key_file_set_string(
            key_file,
            group,
            "shell",
            profile->shell_path != NULL ? profile->shell_path : "");
        g_key_file_set_string(
            key_file,
            group,
            "working-directory",
            profile->working_directory != NULL ? profile->working_directory : "");
        g_key_file_set_string(
            key_file,
            group,
            "theme",
            profile->theme_id != NULL ? profile->theme_id : "follow-system");
        g_key_file_set_uint64(
            key_file,
            group,
            "scrollback-lines",
            MIN(profile->scrollback_lines, GOREE_TERMINAL_SCROLLBACK_MAX));
        g_key_file_set_string(
            key_file,
            group,
            "environment-policy",
            goree_terminal_environment_policy_id(profile->environment_policy));

        if (profile->environment_allowlist != NULL) {
            gsize length = g_strv_length(profile->environment_allowlist);
            g_key_file_set_string_list(
                key_file,
                group,
                "environment-allowlist",
                (const char *const *) profile->environment_allowlist,
                length);
        }
        g_free(group);
    }

    const char *path = resolve_profiles_path();
    if (!ensure_parent_directory(path, error))
        goto failure;

    gsize data_length = 0;
    char *data = g_key_file_to_data(key_file, &data_length, NULL);
    gboolean saved = g_file_set_contents(path, data, (gssize) data_length, error);
    g_free(data);

    if (saved && g_chmod(path, 0600) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to secure profile file: %s",
            g_strerror(saved_errno));
        saved = FALSE;
    }

    g_hash_table_unref(ids);
    g_key_file_unref(key_file);
    return saved;

failure:
    g_hash_table_unref(ids);
    g_key_file_unref(key_file);
    return FALSE;
}
