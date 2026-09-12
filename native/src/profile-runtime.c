#include "profile-runtime.h"

#include <string.h>

static void
set_runtime_error(GError **error, const char *message)
{
    g_set_error_literal(
        error,
        G_OPTION_ERROR,
        G_OPTION_ERROR_BAD_VALUE,
        message);
}

const GoreeTerminalSessionProfile *
goree_terminal_profile_find(const GPtrArray *profiles, const char *profile_id)
{
    if (profiles == NULL || profile_id == NULL || *profile_id == '\0')
        return NULL;

    for (guint i = 0; i < profiles->len; i++) {
        const GoreeTerminalSessionProfile *profile = g_ptr_array_index((GPtrArray *) profiles, i);
        if (profile != NULL && g_strcmp0(profile->id, profile_id) == 0)
            return profile;
    }
    return NULL;
}

gboolean
goree_terminal_profile_runtime_prepare(
    const GoreeTerminalSessionProfile *profile,
    GoreeTerminalProfileRuntime *runtime,
    GError **error)
{
    g_return_val_if_fail(profile != NULL, FALSE);
    g_return_val_if_fail(runtime != NULL, FALSE);

    memset(runtime, 0, sizeof(*runtime));

    if (!goree_terminal_session_profile_validate(profile, error))
        return FALSE;

    gsize environment_count = profile->environment_allowlist != NULL
        ? g_strv_length(profile->environment_allowlist)
        : 0;

    if (environment_count > GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX) {
        set_runtime_error(error, "Profile environment allowlist exceeds the host-session protocol bound.");
        return FALSE;
    }

    if (profile->shell_path != NULL &&
        strlen(profile->shell_path) >= GOREE_TERMINAL_HOST_SHELL_PATH_MAX) {
        set_runtime_error(error, "Profile shell path exceeds the host-session protocol bound.");
        return FALSE;
    }

    if (profile->working_directory != NULL &&
        strlen(profile->working_directory) >= GOREE_TERMINAL_HOST_WORKING_DIRECTORY_MAX) {
        set_runtime_error(error, "Profile working directory exceeds the host-session protocol bound.");
        return FALSE;
    }

    for (gsize i = 0; i < environment_count; i++) {
        if (strlen(profile->environment_allowlist[i]) >=
            GOREE_TERMINAL_HOST_ENVIRONMENT_NAME_MAX) {
            set_runtime_error(error, "Profile environment variable name exceeds the host-session protocol bound.");
            return FALSE;
        }
    }

    runtime->profile = profile;
    runtime->host_context.shell_path = profile->shell_path;
    runtime->host_context.working_directory = profile->working_directory;
    runtime->host_context.environment_names =
        (const char *const *) profile->environment_allowlist;
    runtime->host_context.environment_count = environment_count;
    runtime->host_context.environment_policy =
        profile->environment_policy == GOREE_TERMINAL_ENVIRONMENT_CLEAN
            ? GOREE_TERMINAL_HOST_ENVIRONMENT_CLEAN
            : GOREE_TERMINAL_HOST_ENVIRONMENT_INHERIT_SAFE;
    runtime->scrollback_lines = MIN(
        profile->scrollback_lines,
        GOREE_TERMINAL_SCROLLBACK_MAX);
    runtime->theme_id = profile->theme_id != NULL && *profile->theme_id != '\0'
        ? profile->theme_id
        : "follow-system";
    return TRUE;
}
