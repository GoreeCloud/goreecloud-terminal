#include <glib.h>
#include <string.h>

#include "profile-runtime.h"

static void
test_default_profile_runtime(void)
{
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    GoreeTerminalProfileRuntime runtime;
    GError *error = NULL;

    g_assert_true(goree_terminal_profile_runtime_prepare(profile, &runtime, &error));
    g_assert_no_error(error);
    g_assert_true(runtime.profile == profile);
    g_assert_cmpstr(runtime.theme_id, ==, "follow-system");
    g_assert_cmpuint(runtime.scrollback_lines, ==, GOREE_TERMINAL_SCROLLBACK_DEFAULT);
    g_assert_cmpint(runtime.host_context.environment_policy, ==,
                    GOREE_TERMINAL_HOST_ENVIRONMENT_INHERIT_SAFE);
    g_assert_cmpuint(runtime.host_context.environment_count, ==, 0);
    g_assert_cmpstr(runtime.host_context.shell_path, ==, "");
    g_assert_cmpstr(runtime.host_context.working_directory, ==, "");

    goree_terminal_session_profile_free(profile);
}

static void
test_profile_runtime_maps_bounded_context(void)
{
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    g_free(profile->id);
    profile->id = g_strdup("ops");
    g_free(profile->name);
    profile->name = g_strdup("Operations");
    g_free(profile->shell_path);
    profile->shell_path = g_strdup("/bin/sh");
    g_free(profile->working_directory);
    profile->working_directory = g_strdup("/tmp");
    g_free(profile->theme_id);
    profile->theme_id = g_strdup("deep-dark");
    profile->scrollback_lines = 50000;
    profile->environment_policy = GOREE_TERMINAL_ENVIRONMENT_CLEAN;
    g_strfreev(profile->environment_allowlist);
    profile->environment_allowlist = g_new0(char *, 3);
    profile->environment_allowlist[0] = g_strdup("LANG");
    profile->environment_allowlist[1] = g_strdup("SSH_AUTH_SOCK");

    GoreeTerminalProfileRuntime runtime;
    GError *error = NULL;
    g_assert_true(goree_terminal_profile_runtime_prepare(profile, &runtime, &error));
    g_assert_no_error(error);
    g_assert_cmpint(runtime.host_context.environment_policy, ==,
                    GOREE_TERMINAL_HOST_ENVIRONMENT_CLEAN);
    g_assert_cmpuint(runtime.host_context.environment_count, ==, 2);
    g_assert_cmpstr(runtime.host_context.environment_names[0], ==, "LANG");
    g_assert_cmpstr(runtime.host_context.environment_names[1], ==, "SSH_AUTH_SOCK");
    g_assert_cmpstr(runtime.host_context.shell_path, ==, "/bin/sh");
    g_assert_cmpstr(runtime.host_context.working_directory, ==, "/tmp");
    g_assert_cmpstr(runtime.theme_id, ==, "deep-dark");
    g_assert_cmpuint(runtime.scrollback_lines, ==, 50000);

    goree_terminal_session_profile_free(profile);
}

static void
test_protocol_environment_bound_fails_closed(void)
{
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    g_strfreev(profile->environment_allowlist);
    profile->environment_allowlist = g_new0(
        char *,
        GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX + 2);

    for (guint i = 0; i < GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX + 1; i++)
        profile->environment_allowlist[i] = g_strdup_printf("SAFE_%u", i);

    GoreeTerminalProfileRuntime runtime;
    GError *error = NULL;
    g_assert_false(goree_terminal_profile_runtime_prepare(profile, &runtime, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_session_profile_free(profile);
}

static void
test_profile_find(void)
{
    GPtrArray *profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    g_ptr_array_add(profiles, profile);

    g_assert_true(goree_terminal_profile_find(profiles, "default") == profile);
    g_assert_null(goree_terminal_profile_find(profiles, "missing"));
    g_assert_null(goree_terminal_profile_find(NULL, "default"));

    g_ptr_array_unref(profiles);
}

static GoreeTerminalRuntimeCatalog
build_default_catalog(void)
{
    GoreeTerminalRuntimeCatalog catalog = {0};
    catalog.profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);
    catalog.workspaces = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_workspace_free);
    g_ptr_array_add(catalog.profiles, goree_terminal_session_profile_new_default());
    g_ptr_array_add(catalog.workspaces, goree_terminal_workspace_new_default());
    return catalog;
}

static void
test_runtime_catalog_accepts_valid_defaults(void)
{
    GoreeTerminalRuntimeCatalog catalog = build_default_catalog();
    GError *error = NULL;

    g_assert_true(goree_terminal_runtime_catalog_validate(&catalog, &error));
    g_assert_no_error(error);
    g_assert_nonnull(goree_terminal_runtime_catalog_find_profile(&catalog, "default"));
    g_assert_nonnull(goree_terminal_runtime_catalog_find_workspace(&catalog, "default"));
    g_assert_null(goree_terminal_runtime_catalog_find_profile(&catalog, "missing"));
    g_assert_null(goree_terminal_runtime_catalog_find_workspace(&catalog, "missing"));

    goree_terminal_runtime_catalog_clear(&catalog);
}

static void
test_runtime_catalog_rejects_unknown_profile_reference(void)
{
    GoreeTerminalRuntimeCatalog catalog = build_default_catalog();
    GoreeTerminalWorkspace *workspace = g_ptr_array_index(catalog.workspaces, 0);
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    GError *error = NULL;

    g_free(g_ptr_array_index(tab->profile_ids, 0));
    g_ptr_array_index(tab->profile_ids, 0) = g_strdup("missing");

    g_assert_false(goree_terminal_runtime_catalog_validate(&catalog, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_runtime_catalog_clear(&catalog);
}

static void
test_runtime_catalog_rejects_protocol_overflow(void)
{
    GoreeTerminalRuntimeCatalog catalog = build_default_catalog();
    GoreeTerminalSessionProfile *profile = g_ptr_array_index(catalog.profiles, 0);
    GError *error = NULL;

    g_strfreev(profile->environment_allowlist);
    profile->environment_allowlist = g_new0(
        char *,
        GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX + 2);
    for (guint i = 0; i < GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX + 1; i++)
        profile->environment_allowlist[i] = g_strdup_printf("SAFE_%u", i);

    g_assert_false(goree_terminal_runtime_catalog_validate(&catalog, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_runtime_catalog_clear(&catalog);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/profile-runtime/default", test_default_profile_runtime);
    g_test_add_func("/profile-runtime/context", test_profile_runtime_maps_bounded_context);
    g_test_add_func("/profile-runtime/environment-bound", test_protocol_environment_bound_fails_closed);
    g_test_add_func("/profile-runtime/find", test_profile_find);
    g_test_add_func("/profile-runtime/catalog/defaults", test_runtime_catalog_accepts_valid_defaults);
    g_test_add_func("/profile-runtime/catalog/unknown-profile", test_runtime_catalog_rejects_unknown_profile_reference);
    g_test_add_func("/profile-runtime/catalog/protocol-overflow", test_runtime_catalog_rejects_protocol_overflow);
    return g_test_run();
}
