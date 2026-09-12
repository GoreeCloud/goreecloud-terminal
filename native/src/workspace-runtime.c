#include "workspace-runtime.h"

#include <string.h>

static void
pane_runtime_free(gpointer data)
{
    g_free(data);
}

static void
set_runtime_error(GError **error, const char *format, const char *value)
{
    g_set_error(
        error,
        G_OPTION_ERROR,
        G_OPTION_ERROR_BAD_VALUE,
        format,
        value != NULL ? value : "");
}

gboolean
goree_terminal_workspace_runtime_prepare(
    const GoreeTerminalWorkspace *workspace,
    const GPtrArray *profiles,
    GoreeTerminalWorkspaceRuntime *runtime,
    GError **error)
{
    g_return_val_if_fail(workspace != NULL, FALSE);
    g_return_val_if_fail(profiles != NULL, FALSE);
    g_return_val_if_fail(runtime != NULL, FALSE);

    memset(runtime, 0, sizeof(*runtime));

    if (!goree_terminal_workspace_validate(workspace, error))
        return FALSE;

    runtime->workspace = workspace;
    runtime->tab_count = workspace->tabs != NULL ? workspace->tabs->len : 0;
    runtime->panes = g_ptr_array_new_with_free_func(pane_runtime_free);

    for (guint tab_index = 0; tab_index < runtime->tab_count; tab_index++) {
        const GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, tab_index);
        if (tab == NULL || tab->profile_ids == NULL) {
            set_runtime_error(error, "Workspace tab '%s' is missing its pane profile references.", tab != NULL ? tab->title : "");
            goto failure;
        }

        for (guint pane_index = 0; pane_index < tab->profile_ids->len; pane_index++) {
            const char *profile_id = g_ptr_array_index(tab->profile_ids, pane_index);
            const GoreeTerminalSessionProfile *profile = goree_terminal_profile_find(
                profiles,
                profile_id);
            if (profile == NULL) {
                set_runtime_error(error, "Workspace references unknown terminal profile '%s'.", profile_id);
                goto failure;
            }

            GoreeTerminalWorkspacePaneRuntime *pane = g_new0(
                GoreeTerminalWorkspacePaneRuntime,
                1);
            pane->tab_index = tab_index;
            pane->pane_index = pane_index;
            pane->initially_active = tab_index == 0 && pane_index == 0;
            pane->tab_title = tab->title;
            pane->orientation = tab->orientation;

            if (!goree_terminal_profile_runtime_prepare(
                    profile,
                    &pane->profile_runtime,
                    error)) {
                g_free(pane);
                goto failure;
            }

            g_ptr_array_add(runtime->panes, pane);
        }
    }

    if (runtime->panes->len == 0) {
        set_runtime_error(error, "Workspace '%s' has no launchable panes.", workspace->id);
        goto failure;
    }

    return TRUE;

failure:
    goree_terminal_workspace_runtime_clear(runtime);
    return FALSE;
}

void
goree_terminal_workspace_runtime_clear(GoreeTerminalWorkspaceRuntime *runtime)
{
    if (runtime == NULL)
        return;

    g_clear_pointer(&runtime->panes, g_ptr_array_unref);
    runtime->workspace = NULL;
    runtime->tab_count = 0;
}
