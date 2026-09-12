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
    runtime->host_context.shell_path = profile->shell_path != NULL
        ? profile->shell_path
        : "";
    runtime->host_context.working_directory = profile->working_directory != NULL
        ? profile->working_directory
        : "";
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

const GoreeTerminalSessionProfile *
goree_terminal_runtime_catalog_find_profile(
    const GoreeTerminalRuntimeCatalog *catalog,
    const char *profile_id)
{
    if (catalog == NULL)
        return NULL;
    return goree_terminal_profile_find(catalog->profiles, profile_id);
}

const GoreeTerminalWorkspace *
goree_terminal_runtime_catalog_find_workspace(
    const GoreeTerminalRuntimeCatalog *catalog,
    const char *workspace_id)
{
    if (catalog == NULL || catalog->workspaces == NULL || workspace_id == NULL)
        return NULL;

    for (guint index = 0; index < catalog->workspaces->len; index++) {
        GoreeTerminalWorkspace *workspace = g_ptr_array_index(
            catalog->workspaces,
            index);
        if (workspace != NULL && g_strcmp0(workspace->id, workspace_id) == 0)
            return workspace;
    }
    return NULL;
}

gboolean
goree_terminal_runtime_catalog_validate(
    const GoreeTerminalRuntimeCatalog *catalog,
    GError **error)
{
    if (catalog == NULL || catalog->profiles == NULL ||
        catalog->workspaces == NULL || catalog->profiles->len == 0 ||
        catalog->workspaces->len == 0) {
        set_runtime_error(error, "Terminal runtime catalog is incomplete.");
        return FALSE;
    }

    GHashTable *profile_ids = g_hash_table_new(g_str_hash, g_str_equal);
    for (guint index = 0; index < catalog->profiles->len; index++) {
        GoreeTerminalSessionProfile *profile = g_ptr_array_index(
            catalog->profiles,
            index);
        GoreeTerminalProfileRuntime runtime;

        if (!goree_terminal_profile_runtime_prepare(profile, &runtime, error)) {
            g_hash_table_unref(profile_ids);
            return FALSE;
        }
        if (g_hash_table_contains(profile_ids, profile->id)) {
            set_runtime_error(error, "Terminal runtime catalog contains duplicate profile IDs.");
            g_hash_table_unref(profile_ids);
            return FALSE;
        }
        g_hash_table_add(profile_ids, profile->id);
    }

    GHashTable *workspace_ids = g_hash_table_new(g_str_hash, g_str_equal);
    for (guint index = 0; index < catalog->workspaces->len; index++) {
        GoreeTerminalWorkspace *workspace = g_ptr_array_index(
            catalog->workspaces,
            index);
        if (!goree_terminal_workspace_validate(workspace, error)) {
            g_hash_table_unref(profile_ids);
            g_hash_table_unref(workspace_ids);
            return FALSE;
        }
        if (g_hash_table_contains(workspace_ids, workspace->id)) {
            set_runtime_error(error, "Terminal runtime catalog contains duplicate workspace IDs.");
            g_hash_table_unref(profile_ids);
            g_hash_table_unref(workspace_ids);
            return FALSE;
        }
        g_hash_table_add(workspace_ids, workspace->id);

        for (guint tab_index = 0; tab_index < workspace->tabs->len; tab_index++) {
            GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(
                workspace->tabs,
                tab_index);
            for (guint pane_index = 0;
                 pane_index < tab->profile_ids->len;
                 pane_index++) {
                const char *profile_id = g_ptr_array_index(
                    tab->profile_ids,
                    pane_index);
                if (!g_hash_table_contains(profile_ids, profile_id)) {
                    set_runtime_error(
                        error,
                        "Workspace references a profile that is not present in the runtime catalog.");
                    g_hash_table_unref(profile_ids);
                    g_hash_table_unref(workspace_ids);
                    return FALSE;
                }
            }
        }
    }

    g_hash_table_unref(profile_ids);
    g_hash_table_unref(workspace_ids);
    return TRUE;
}

gboolean
goree_terminal_runtime_catalog_load(
    GoreeTerminalRuntimeCatalog *catalog,
    GError **error)
{
    g_return_val_if_fail(catalog != NULL, FALSE);

    memset(catalog, 0, sizeof(*catalog));
    catalog->profiles = goree_terminal_session_profiles_load(error);
    if (catalog->profiles == NULL)
        return FALSE;

    catalog->workspaces = goree_terminal_workspaces_load(error);
    if (catalog->workspaces == NULL) {
        g_clear_pointer(&catalog->profiles, g_ptr_array_unref);
        return FALSE;
    }

    if (!goree_terminal_runtime_catalog_validate(catalog, error)) {
        goree_terminal_runtime_catalog_clear(catalog);
        return FALSE;
    }
    return TRUE;
}

void
goree_terminal_runtime_catalog_clear(GoreeTerminalRuntimeCatalog *catalog)
{
    if (catalog == NULL)
        return;

    g_clear_pointer(&catalog->profiles, g_ptr_array_unref);
    g_clear_pointer(&catalog->workspaces, g_ptr_array_unref);
}

void
goree_terminal_runtime_launch_context_for_profile(
    const GoreeTerminalSessionProfile *profile,
    GoreeTerminalHostLaunchContext *context)
{
    g_return_if_fail(profile != NULL);
    g_return_if_fail(context != NULL);

    GoreeTerminalProfileRuntime runtime;
    GError *error = NULL;
    if (!goree_terminal_profile_runtime_prepare(profile, &runtime, &error)) {
        g_warning(
            "Validated Terminal profile could not be converted to a host launch context: %s",
            error != NULL ? error->message : "unknown error");
        g_clear_error(&error);
        memset(context, 0, sizeof(*context));
        return;
    }

    *context = runtime.host_context;
}
