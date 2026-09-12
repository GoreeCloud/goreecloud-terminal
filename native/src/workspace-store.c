#include "workspace-store.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <string.h>

#define WORKSPACE_FILE_NAME "workspaces.ini"
#define WORKSPACE_GROUP_PREFIX "Workspace:"
#define WORKSPACE_ID_MAX 64
#define WORKSPACE_NAME_MAX 128
#define WORKSPACE_TAB_MAX 16
#define WORKSPACE_PANE_MAX 4

static char *workspaces_path = NULL;

static const char *
resolve_workspaces_path(void)
{
    const char *override = g_getenv("GOREE_TERMINAL_WORKSPACES_PATH");

    if (override != NULL && *override != '\0')
        return override;

    if (workspaces_path == NULL) {
        workspaces_path = g_build_filename(
            g_get_user_config_dir(),
            "goreecloud",
            "terminal",
            WORKSPACE_FILE_NAME,
            NULL);
    }
    return workspaces_path;
}

const char *
goree_terminal_workspaces_path(void)
{
    return resolve_workspaces_path();
}

static void
set_validation_error(GError **error, const char *message)
{
    g_set_error_literal(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE, message);
}

static gboolean
valid_identifier(const char *id)
{
    if (id == NULL || *id == '\0' || strlen(id) > WORKSPACE_ID_MAX)
        return FALSE;

    for (const char *cursor = id; *cursor != '\0'; cursor++) {
        if (!(g_ascii_islower(*cursor) || g_ascii_isdigit(*cursor) ||
              *cursor == '-' || *cursor == '_'))
            return FALSE;
    }
    return TRUE;
}

GoreeTerminalWorkspaceTab *
goree_terminal_workspace_tab_new(
    const char *title,
    GoreeTerminalSplitOrientation orientation)
{
    GoreeTerminalWorkspaceTab *tab = g_new0(GoreeTerminalWorkspaceTab, 1);
    tab->title = g_strdup(title != NULL ? title : "Terminal");
    tab->orientation = orientation;
    tab->profile_ids = g_ptr_array_new_with_free_func(g_free);
    return tab;
}

void
goree_terminal_workspace_tab_free(GoreeTerminalWorkspaceTab *tab)
{
    if (tab == NULL)
        return;
    g_free(tab->title);
    g_clear_pointer(&tab->profile_ids, g_ptr_array_unref);
    g_free(tab);
}

GoreeTerminalWorkspace *
goree_terminal_workspace_new_default(void)
{
    GoreeTerminalWorkspace *workspace = g_new0(GoreeTerminalWorkspace, 1);
    workspace->id = g_strdup("default");
    workspace->name = g_strdup("Default Workspace");
    workspace->tabs = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_workspace_tab_free);

    GoreeTerminalWorkspaceTab *tab = goree_terminal_workspace_tab_new(
        "Terminal",
        GOREE_TERMINAL_SPLIT_HORIZONTAL);
    g_ptr_array_add(tab->profile_ids, g_strdup("default"));
    g_ptr_array_add(workspace->tabs, tab);
    return workspace;
}

void
goree_terminal_workspace_free(GoreeTerminalWorkspace *workspace)
{
    if (workspace == NULL)
        return;
    g_free(workspace->id);
    g_free(workspace->name);
    g_clear_pointer(&workspace->tabs, g_ptr_array_unref);
    g_free(workspace);
}

const char *
goree_terminal_split_orientation_id(GoreeTerminalSplitOrientation orientation)
{
    return orientation == GOREE_TERMINAL_SPLIT_VERTICAL ? "vertical" : "horizontal";
}

gboolean
goree_terminal_split_orientation_from_id(
    const char *id,
    GoreeTerminalSplitOrientation *orientation)
{
    g_return_val_if_fail(orientation != NULL, FALSE);

    if (g_strcmp0(id, "horizontal") == 0) {
        *orientation = GOREE_TERMINAL_SPLIT_HORIZONTAL;
        return TRUE;
    }
    if (g_strcmp0(id, "vertical") == 0) {
        *orientation = GOREE_TERMINAL_SPLIT_VERTICAL;
        return TRUE;
    }
    return FALSE;
}

gboolean
goree_terminal_workspace_validate(
    const GoreeTerminalWorkspace *workspace,
    GError **error)
{
    g_return_val_if_fail(workspace != NULL, FALSE);

    if (!valid_identifier(workspace->id)) {
        set_validation_error(error, "Workspace ID is invalid.");
        return FALSE;
    }
    if (workspace->name == NULL || *workspace->name == '\0' ||
        !g_utf8_validate(workspace->name, -1, NULL) ||
        g_utf8_strlen(workspace->name, -1) > WORKSPACE_NAME_MAX) {
        set_validation_error(error, "Workspace name is missing, invalid, or too long.");
        return FALSE;
    }
    if (workspace->tabs == NULL || workspace->tabs->len == 0 ||
        workspace->tabs->len > WORKSPACE_TAB_MAX) {
        set_validation_error(error, "Workspace must contain between 1 and 16 tabs.");
        return FALSE;
    }

    for (guint i = 0; i < workspace->tabs->len; i++) {
        GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, i);
        if (tab == NULL || tab->profile_ids == NULL ||
            tab->profile_ids->len == 0 || tab->profile_ids->len > WORKSPACE_PANE_MAX) {
            set_validation_error(error, "Workspace tabs must contain between 1 and 4 panes.");
            return FALSE;
        }
        if (tab->title == NULL || !g_utf8_validate(tab->title, -1, NULL)) {
            set_validation_error(error, "Workspace tab title is invalid.");
            return FALSE;
        }
        if (tab->orientation != GOREE_TERMINAL_SPLIT_HORIZONTAL &&
            tab->orientation != GOREE_TERMINAL_SPLIT_VERTICAL) {
            set_validation_error(error, "Workspace split orientation is invalid.");
            return FALSE;
        }
        for (guint pane = 0; pane < tab->profile_ids->len; pane++) {
            const char *profile_id = g_ptr_array_index(tab->profile_ids, pane);
            if (!valid_identifier(profile_id)) {
                set_validation_error(error, "Workspace references an invalid profile ID.");
                return FALSE;
            }
        }
    }

    return TRUE;
}

static GoreeTerminalWorkspace *
load_workspace(GKeyFile *key_file, const char *group, GError **error)
{
    GoreeTerminalWorkspace *workspace = g_new0(GoreeTerminalWorkspace, 1);
    workspace->id = g_strdup(group + strlen(WORKSPACE_GROUP_PREFIX));
    workspace->name = g_key_file_get_string(key_file, group, "name", error);
    workspace->tabs = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_workspace_tab_free);
    if (workspace->name == NULL)
        goto failure;

    GError *count_error = NULL;
    guint64 tab_count = g_key_file_get_uint64(key_file, group, "tab-count", &count_error);
    if (count_error != NULL || tab_count == 0 || tab_count > WORKSPACE_TAB_MAX) {
        g_clear_error(&count_error);
        set_validation_error(error, "Workspace tab count is missing or invalid.");
        goto failure;
    }

    for (guint64 i = 0; i < tab_count; i++) {
        char *title_key = g_strdup_printf("tab-%" G_GUINT64_FORMAT "-title", i);
        char *orientation_key = g_strdup_printf("tab-%" G_GUINT64_FORMAT "-orientation", i);
        char *profiles_key = g_strdup_printf("tab-%" G_GUINT64_FORMAT "-profiles", i);
        char *title = g_key_file_get_string(key_file, group, title_key, NULL);
        char *orientation_id = g_key_file_get_string(key_file, group, orientation_key, NULL);
        gsize profile_count = 0;
        char **profile_ids = g_key_file_get_string_list(
            key_file,
            group,
            profiles_key,
            &profile_count,
            NULL);
        GoreeTerminalSplitOrientation orientation;

        g_free(title_key);
        g_free(orientation_key);
        g_free(profiles_key);

        if (title == NULL)
            title = g_strdup("Terminal");
        if (!goree_terminal_split_orientation_from_id(orientation_id, &orientation) ||
            profile_ids == NULL || profile_count == 0 || profile_count > WORKSPACE_PANE_MAX) {
            g_free(title);
            g_free(orientation_id);
            g_strfreev(profile_ids);
            set_validation_error(error, "Workspace tab layout is invalid.");
            goto failure;
        }

        GoreeTerminalWorkspaceTab *tab = goree_terminal_workspace_tab_new(title, orientation);
        for (gsize pane = 0; pane < profile_count; pane++)
            g_ptr_array_add(tab->profile_ids, g_strdup(profile_ids[pane]));
        g_ptr_array_add(workspace->tabs, tab);

        g_free(title);
        g_free(orientation_id);
        g_strfreev(profile_ids);
    }

    if (!goree_terminal_workspace_validate(workspace, error))
        goto failure;
    return workspace;

failure:
    goree_terminal_workspace_free(workspace);
    return NULL;
}

GPtrArray *
goree_terminal_workspaces_load(GError **error)
{
    GPtrArray *workspaces = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_workspace_free);
    GKeyFile *key_file = g_key_file_new();
    GError *local_error = NULL;

    if (!g_key_file_load_from_file(
            key_file,
            resolve_workspaces_path(),
            G_KEY_FILE_NONE,
            &local_error)) {
        if (g_error_matches(local_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_clear_error(&local_error);
            g_key_file_unref(key_file);
            g_ptr_array_add(workspaces, goree_terminal_workspace_new_default());
            return workspaces;
        }
        g_propagate_error(error, local_error);
        g_key_file_unref(key_file);
        g_ptr_array_unref(workspaces);
        return NULL;
    }

    gsize group_count = 0;
    char **groups = g_key_file_get_groups(key_file, &group_count);
    for (gsize i = 0; i < group_count; i++) {
        if (!g_str_has_prefix(groups[i], WORKSPACE_GROUP_PREFIX))
            continue;
        GoreeTerminalWorkspace *workspace = load_workspace(key_file, groups[i], error);
        if (workspace == NULL) {
            g_strfreev(groups);
            g_key_file_unref(key_file);
            g_ptr_array_unref(workspaces);
            return NULL;
        }
        g_ptr_array_add(workspaces, workspace);
    }

    g_strfreev(groups);
    g_key_file_unref(key_file);
    if (workspaces->len == 0)
        g_ptr_array_add(workspaces, goree_terminal_workspace_new_default());
    return workspaces;
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
            "Unable to create workspace directory: %s",
            g_strerror(saved_errno));
        g_free(directory);
        return FALSE;
    }
    g_free(directory);
    return TRUE;
}

gboolean
goree_terminal_workspaces_save(
    const GPtrArray *workspaces,
    GError **error)
{
    g_return_val_if_fail(workspaces != NULL, FALSE);
    if (workspaces->len == 0) {
        set_validation_error(error, "At least one workspace is required.");
        return FALSE;
    }

    GHashTable *ids = g_hash_table_new(g_str_hash, g_str_equal);
    GKeyFile *key_file = g_key_file_new();

    for (guint w = 0; w < workspaces->len; w++) {
        GoreeTerminalWorkspace *workspace = g_ptr_array_index(workspaces, w);
        if (!goree_terminal_workspace_validate(workspace, error))
            goto failure;
        if (g_hash_table_contains(ids, workspace->id)) {
            set_validation_error(error, "Workspace IDs must be unique.");
            goto failure;
        }
        g_hash_table_add(ids, workspace->id);

        char *group = g_strdup_printf("%s%s", WORKSPACE_GROUP_PREFIX, workspace->id);
        g_key_file_set_string(key_file, group, "name", workspace->name);
        g_key_file_set_uint64(key_file, group, "tab-count", workspace->tabs->len);

        for (guint i = 0; i < workspace->tabs->len; i++) {
            GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, i);
            char *title_key = g_strdup_printf("tab-%u-title", i);
            char *orientation_key = g_strdup_printf("tab-%u-orientation", i);
            char *profiles_key = g_strdup_printf("tab-%u-profiles", i);
            const char **profiles = g_new0(const char *, tab->profile_ids->len);

            for (guint pane = 0; pane < tab->profile_ids->len; pane++)
                profiles[pane] = g_ptr_array_index(tab->profile_ids, pane);

            g_key_file_set_string(key_file, group, title_key, tab->title);
            g_key_file_set_string(
                key_file,
                group,
                orientation_key,
                goree_terminal_split_orientation_id(tab->orientation));
            g_key_file_set_string_list(
                key_file,
                group,
                profiles_key,
                profiles,
                tab->profile_ids->len);

            g_free(profiles);
            g_free(title_key);
            g_free(orientation_key);
            g_free(profiles_key);
        }
        g_free(group);
    }

    const char *path = resolve_workspaces_path();
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
            "Unable to secure workspace file: %s",
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
