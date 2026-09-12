#include "profile-runtime.h"

#include <glib.h>

static GoreeTerminalSessionProfile *
profile_new(const char *id)
{
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    g_free(profile->id);
    g_free(profile->name);
    profile->id = g_strdup(id);
    profile->name = g_strdup(id);
    return profile;
}

static GoreeTerminalWorkspace *
workspace_new(const char *id, const char *profile_id)
{
    GoreeTerminalWorkspace *workspace = goree_terminal_workspace_new_default();
    g_free(workspace->id);
    g_free(workspace->name);
    workspace->id = g_strdup(id);
    workspace->name = g_strdup(id);

    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    g_ptr_array_set_size(tab->profile_ids, 0);
    g_ptr_array_add(tab->profile_ids, g_strdup(profile_id));
    return workspace;
}

static GoreeTerminalRuntimeCatalog
catalog_new(void)
{
    GoreeTerminalRuntimeCatalog catalog = {
        .profiles = g_ptr_array_new_with_free_func(
            (GDestroyNotify) goree_terminal_session_profile_free),
        .workspaces = g_ptr_array_new_with_free_func(
            (GDestroyNotify) goree_terminal_workspace_free),
    };
    return catalog;
}

static void
test_catalog_accepts_bound_profiles(void)
{
    GoreeTerminalRuntimeCatalog catalog = catalog_new();
    g_ptr_array_add(catalog.profiles, profile_new("default"));
    g_ptr_array_add(catalog.workspaces, workspace_new("default", "default"));

    GError *error = NULL;
    g_assert_true(goree_terminal_runtime_catalog_validate(&catalog, &error));
    g_assert_no_error(error);
    g_assert_nonnull(goree_terminal_runtime_catalog_find_profile(&catalog, "default"));
    g_assert_nonnull(goree_terminal_runtime_catalog_find_workspace(&catalog, "default"));

    goree_terminal_runtime_catalog_clear(&catalog);
}

static void
test_catalog_rejects_missing_profile(void)
{
    GoreeTerminalRuntimeCatalog catalog = catalog_new();
    g_ptr_array_add(catalog.profiles, profile_new("default"));
    g_ptr_array_add(catalog.workspaces, workspace_new("broken", "missing"));

    GError *error = NULL;
    g_assert_false(goree_terminal_runtime_catalog_validate(&catalog, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    goree_terminal_runtime_catalog_clear(&catalog);
}

static void
test_catalog_rejects_duplicate_profiles(void)
{
    GoreeTerminalRuntimeCatalog catalog = catalog_new();
    g_ptr_array_add(catalog.profiles, profile_new("same"));
    g_ptr_array_add(catalog.profiles, profile_new("same"));
    g_ptr_array_add(catalog.workspaces, workspace_new("default", "same"));

    GError *error = NULL;
    g_assert_false(goree_terminal_runtime_catalog_validate(&catalog, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    goree_terminal_runtime_catalog_clear(&catalog);
}

static void
test_launch_context_contains_names_not_values(void)
{
    GoreeTerminalSessionProfile *profile = profile_new("ops");
    g_free(profile->shell_path);
    g_free(profile->working_directory);
    profile->shell_path = g_strdup("/bin/sh");
    profile->working_directory = g_strdup("/tmp");
    profile->environment_policy = GOREE_TERMINAL_ENVIRONMENT_CLEAN;
    g_strfreev(profile->environment_allowlist);
    profile->environment_allowlist = g_new0(char *, 3);
    profile->environment_allowlist[0] = g_strdup("SSH_AUTH_SOCK");
    profile->environment_allowlist[1] = g_strdup("LANG");

    GoreeTerminalHostLaunchContext context = {0};
    goree_terminal_runtime_launch_context_for_profile(profile, &context);

    g_assert_cmpstr(context.shell_path, ==, "/bin/sh");
    g_assert_cmpstr(context.working_directory, ==, "/tmp");
    g_assert_cmpint(context.environment_policy, ==, GOREE_TERMINAL_HOST_ENVIRONMENT_CLEAN);
    g_assert_cmpuint(context.environment_count, ==, 2);
    g_assert_cmpstr(context.environment_names[0], ==, "SSH_AUTH_SOCK");
    g_assert_cmpstr(context.environment_names[1], ==, "LANG");

    goree_terminal_session_profile_free(profile);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/profile-runtime/catalog-valid", test_catalog_accepts_bound_profiles);
    g_test_add_func("/profile-runtime/missing-profile", test_catalog_rejects_missing_profile);
    g_test_add_func("/profile-runtime/duplicate-profile", test_catalog_rejects_duplicate_profiles);
    g_test_add_func("/profile-runtime/launch-context", test_launch_context_contains_names_not_values);
    return g_test_run();
}
