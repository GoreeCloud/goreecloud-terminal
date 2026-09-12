#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>

#include "workspace-store.h"

static char *test_directory = NULL;
static char *test_path = NULL;

static void
reset_file(void)
{
    if (test_path != NULL)
        g_remove(test_path);
}

static void
test_missing_file_returns_default(void)
{
    GError *error = NULL;
    reset_file();

    GPtrArray *workspaces = goree_terminal_workspaces_load(&error);
    g_assert_no_error(error);
    g_assert_nonnull(workspaces);
    g_assert_cmpuint(workspaces->len, ==, 1);

    GoreeTerminalWorkspace *workspace = g_ptr_array_index(workspaces, 0);
    g_assert_cmpstr(workspace->id, ==, "default");
    g_assert_cmpuint(workspace->tabs->len, ==, 1);
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    g_assert_cmpuint(tab->profile_ids->len, ==, 1);
    g_assert_cmpstr(g_ptr_array_index(tab->profile_ids, 0), ==, "default");
    g_ptr_array_unref(workspaces);
}

static void
test_round_trip_and_permissions(void)
{
    GError *error = NULL;
    GPtrArray *workspaces = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_workspace_free);
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    g_free(workspace->id);
    workspace->id = g_strdup("ops");
    g_free(workspace->name);
    workspace->name = g_strdup("Operations");

    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    g_free(tab->title);
    tab->title = g_strdup("Local and Remote");
    tab->orientation = GOREE_TERMINAL_SPLIT_VERTICAL;
    g_ptr_array_add(tab->profile_ids, g_strdup("remote-admin"));
    g_ptr_array_add(workspaces, workspace);

    g_assert_true(goree_terminal_workspaces_save(workspaces, &error));
    g_assert_no_error(error);

    struct stat file_stat;
    g_assert_cmpint(g_stat(test_path, &file_stat), ==, 0);
    g_assert_cmpuint(file_stat.st_mode & 0777, ==, 0600);

    GPtrArray *loaded = goree_terminal_workspaces_load(&error);
    g_assert_no_error(error);
    g_assert_cmpuint(loaded->len, ==, 1);
    GoreeTerminalWorkspace *loaded_workspace = g_ptr_array_index(loaded, 0);
    g_assert_cmpstr(loaded_workspace->name, ==, "Operations");
    GoreeTerminalWorkspaceTab *loaded_tab = g_ptr_array_index(loaded_workspace->tabs, 0);
    g_assert_cmpint(loaded_tab->orientation, ==, GOREE_TERMINAL_SPLIT_VERTICAL);
    g_assert_cmpuint(loaded_tab->profile_ids->len, ==, 2);
    g_assert_cmpstr(g_ptr_array_index(loaded_tab->profile_ids, 1), ==, "remote-admin");

    g_ptr_array_unref(loaded);
    g_ptr_array_unref(workspaces);
}

static void
test_too_many_panes_fail(void)
{
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    GError *error = NULL;

    for (guint i = 0; i < 4; i++)
        g_ptr_array_add(tab->profile_ids, g_strdup("default"));

    g_assert_cmpuint(tab->profile_ids->len, ==, 5);
    g_assert_false(goree_terminal_workspace_validate(workspace, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_workspace_free(workspace);
}

static void
test_invalid_profile_reference_fails(void)
{
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    GError *error = NULL;

    g_free(g_ptr_array_index(tab->profile_ids, 0));
    g_ptr_array_index(tab->profile_ids, 0) = g_strdup("../bad-profile");
    g_assert_false(goree_terminal_workspace_validate(workspace, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_workspace_free(workspace);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    test_directory = g_dir_make_tmp("goreecloud-terminal-workspaces-XXXXXX", NULL);
    g_assert_nonnull(test_directory);
    test_path = g_build_filename(test_directory, "workspaces.ini", NULL);
    g_setenv("GOREE_TERMINAL_WORKSPACES_PATH", test_path, TRUE);

    g_test_add_func("/workspaces/default", test_missing_file_returns_default);
    g_test_add_func("/workspaces/round-trip", test_round_trip_and_permissions);
    g_test_add_func("/workspaces/pane-bound", test_too_many_panes_fail);
    g_test_add_func("/workspaces/profile-reference", test_invalid_profile_reference_fails);

    int status = g_test_run();
    reset_file();
    g_rmdir(test_directory);
    g_free(test_path);
    g_free(test_directory);
    return status;
}
