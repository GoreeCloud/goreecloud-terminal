#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>

#include "session-profile.h"

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

    GPtrArray *profiles = goree_terminal_session_profiles_load(&error);
    g_assert_no_error(error);
    g_assert_nonnull(profiles);
    g_assert_cmpuint(profiles->len, ==, 1);

    GoreeTerminalSessionProfile *profile = g_ptr_array_index(profiles, 0);
    g_assert_cmpstr(profile->id, ==, "default");
    g_assert_cmpstr(profile->name, ==, "Default");
    g_assert_cmpint(
        profile->environment_policy,
        ==,
        GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE);
    g_ptr_array_unref(profiles);
}

static void
test_round_trip_and_permissions(void)
{
    GError *error = NULL;
    GPtrArray *profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();

    g_free(profile->id);
    profile->id = g_strdup("development");
    g_free(profile->name);
    profile->name = g_strdup("Development");
    g_free(profile->shell_path);
    profile->shell_path = g_strdup("/bin/bash");
    g_free(profile->working_directory);
    profile->working_directory = g_strdup("/tmp");
    profile->scrollback_lines = 50000;
    profile->environment_policy = GOREE_TERMINAL_ENVIRONMENT_CLEAN;
    g_strfreev(profile->environment_allowlist);
    profile->environment_allowlist = g_new0(char *, 3);
    profile->environment_allowlist[0] = g_strdup("LANG");
    profile->environment_allowlist[1] = g_strdup("TERM");
    g_ptr_array_add(profiles, profile);

    g_assert_true(goree_terminal_session_profiles_save(profiles, &error));
    g_assert_no_error(error);

    struct stat file_stat;
    g_assert_cmpint(g_stat(test_path, &file_stat), ==, 0);
    g_assert_cmpuint(file_stat.st_mode & 0777, ==, 0600);

    GPtrArray *loaded = goree_terminal_session_profiles_load(&error);
    g_assert_no_error(error);
    g_assert_cmpuint(loaded->len, ==, 1);
    GoreeTerminalSessionProfile *loaded_profile = g_ptr_array_index(loaded, 0);
    g_assert_cmpstr(loaded_profile->id, ==, "development");
    g_assert_cmpstr(loaded_profile->shell_path, ==, "/bin/bash");
    g_assert_cmpstr(loaded_profile->working_directory, ==, "/tmp");
    g_assert_cmpuint(loaded_profile->scrollback_lines, ==, 50000);
    g_assert_cmpstr(loaded_profile->environment_allowlist[0], ==, "LANG");
    g_assert_cmpstr(loaded_profile->environment_allowlist[1], ==, "TERM");
    g_assert_null(loaded_profile->environment_allowlist[2]);

    g_ptr_array_unref(loaded);
    g_ptr_array_unref(profiles);
}

static void
test_profile_rejects_environment_values(void)
{
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    GError *error = NULL;

    g_strfreev(profile->environment_allowlist);
    profile->environment_allowlist = g_new0(char *, 2);
    profile->environment_allowlist[0] = g_strdup("TOKEN=secret");

    g_assert_false(goree_terminal_session_profile_validate(profile, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_session_profile_free(profile);
}

static void
test_profile_rejects_relative_launch_paths(void)
{
    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    GError *error = NULL;

    g_free(profile->working_directory);
    profile->working_directory = g_strdup("relative/path");
    g_assert_false(goree_terminal_session_profile_validate(profile, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    g_free(profile->working_directory);
    profile->working_directory = g_strdup("");
    g_free(profile->shell_path);
    profile->shell_path = g_strdup("bash");
    g_assert_false(goree_terminal_session_profile_validate(profile, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    goree_terminal_session_profile_free(profile);
}

static void
test_duplicate_profile_ids_fail(void)
{
    GError *error = NULL;
    GPtrArray *profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);
    g_ptr_array_add(profiles, goree_terminal_session_profile_new_default());
    g_ptr_array_add(profiles, goree_terminal_session_profile_new_default());

    g_assert_false(goree_terminal_session_profiles_save(profiles, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    g_ptr_array_unref(profiles);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    test_directory = g_dir_make_tmp("goreecloud-terminal-profiles-XXXXXX", NULL);
    g_assert_nonnull(test_directory);
    test_path = g_build_filename(test_directory, "profiles.ini", NULL);
    g_setenv("GOREE_TERMINAL_PROFILES_PATH", test_path, TRUE);

    g_test_add_func("/profiles/default", test_missing_file_returns_default);
    g_test_add_func("/profiles/round-trip", test_round_trip_and_permissions);
    g_test_add_func("/profiles/no-environment-values", test_profile_rejects_environment_values);
    g_test_add_func("/profiles/absolute-paths", test_profile_rejects_relative_launch_paths);
    g_test_add_func("/profiles/unique-ids", test_duplicate_profile_ids_fail);

    int status = g_test_run();
    reset_file();
    g_rmdir(test_directory);
    g_free(test_path);
    g_free(test_directory);
    return status;
}
