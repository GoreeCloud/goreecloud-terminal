#include <glib.h>

#include "workspace-runtime.h"

static GPtrArray *
build_profiles(void)
{
    GPtrArray *profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);

    GoreeTerminalSessionProfile *default_profile =
        goree_terminal_session_profile_new_default();
    g_ptr_array_add(profiles, default_profile);

    GoreeTerminalSessionProfile *ops =
        goree_terminal_session_profile_new_default();
    g_free(ops->id);
    ops->id = g_strdup("ops");
    g_free(ops->name);
    ops->name = g_strdup("Operations");
    g_free(ops->theme_id);
    ops->theme_id = g_strdup("deep-dark");
    g_ptr_array_add(profiles, ops);
    return profiles;
}

static void
test_default_workspace_runtime(void)
{
    GPtrArray *profiles = build_profiles();
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    GoreeTerminalWorkspaceRuntime runtime;
    GError *error = NULL;

    g_assert_true(goree_terminal_workspace_runtime_prepare(
        workspace,
        profiles,
        &runtime,
        &error));
    g_assert_no_error(error);
    g_assert_cmpuint(runtime.tab_count, ==, 1);
    g_assert_cmpuint(runtime.panes->len, ==, 1);

    GoreeTerminalWorkspacePaneRuntime *pane = g_ptr_array_index(runtime.panes, 0);
    g_assert_cmpuint(pane->tab_index, ==, 0);
    g_assert_cmpuint(pane->pane_index, ==, 0);
    g_assert_true(pane->initially_active);
    g_assert_cmpstr(pane->profile_runtime.profile->id, ==, "default");

    goree_terminal_workspace_runtime_clear(&runtime);
    goree_terminal_workspace_free(workspace);
    g_ptr_array_unref(profiles);
}

static void
test_split_workspace_runtime_preserves_order(void)
{
    GPtrArray *profiles = build_profiles();
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    tab->orientation = GOREE_TERMINAL_SPLIT_VERTICAL;
    g_ptr_array_add(tab->profile_ids, g_strdup("ops"));

    GoreeTerminalWorkspaceRuntime runtime;
    GError *error = NULL;
    g_assert_true(goree_terminal_workspace_runtime_prepare(
        workspace,
        profiles,
        &runtime,
        &error));
    g_assert_no_error(error);
    g_assert_cmpuint(runtime.panes->len, ==, 2);

    GoreeTerminalWorkspacePaneRuntime *first = g_ptr_array_index(runtime.panes, 0);
    GoreeTerminalWorkspacePaneRuntime *second = g_ptr_array_index(runtime.panes, 1);
    g_assert_true(first->initially_active);
    g_assert_false(second->initially_active);
    g_assert_cmpint(first->orientation, ==, GOREE_TERMINAL_SPLIT_VERTICAL);
    g_assert_cmpint(second->orientation, ==, GOREE_TERMINAL_SPLIT_VERTICAL);
    g_assert_cmpstr(first->profile_runtime.profile->id, ==, "default");
    g_assert_cmpstr(second->profile_runtime.profile->id, ==, "ops");
    g_assert_cmpstr(second->profile_runtime.theme_id, ==, "deep-dark");

    goree_terminal_workspace_runtime_clear(&runtime);
    goree_terminal_workspace_free(workspace);
    g_ptr_array_unref(profiles);
}

static void
test_unknown_profile_fails_closed(void)
{
    GPtrArray *profiles = build_profiles();
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    g_free(g_ptr_array_index(tab->profile_ids, 0));
    g_ptr_array_index(tab->profile_ids, 0) = g_strdup("missing");

    GoreeTerminalWorkspaceRuntime runtime;
    GError *error = NULL;
    g_assert_false(goree_terminal_workspace_runtime_prepare(
        workspace,
        profiles,
        &runtime,
        &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    g_assert_null(runtime.panes);

    goree_terminal_workspace_free(workspace);
    g_ptr_array_unref(profiles);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/workspace-runtime/default", test_default_workspace_runtime);
    g_test_add_func("/workspace-runtime/split-order", test_split_workspace_runtime_preserves_order);
    g_test_add_func("/workspace-runtime/unknown-profile", test_unknown_profile_fails_closed);
    return g_test_run();
}
