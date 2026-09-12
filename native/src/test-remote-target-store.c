#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <sys/stat.h>

#include "remote-target-store.h"

static char *test_directory = NULL;
static char *test_path = NULL;

static void
reset_file(void)
{
    if (test_path != NULL)
        g_remove(test_path);
}

static GPtrArray *
new_record_array(void)
{
    return g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_remote_target_record_free);
}

static void
test_missing_file_returns_empty(void)
{
    reset_file();
    GError *error = NULL;
    GPtrArray *records = goree_terminal_remote_targets_load(&error);
    g_assert_no_error(error);
    g_assert_nonnull(records);
    g_assert_cmpuint(records->len, ==, 0);
    g_ptr_array_unref(records);
}

static void
test_round_trip_private_metadata_only(void)
{
    reset_file();
    GError *error = NULL;
    GPtrArray *records = new_record_array();
    GoreeTerminalRemoteTargetRecord *record = goree_terminal_remote_target_record_new(
        "ops",
        "Operations",
        "admin.example.internal",
        "operator",
        2222,
        &error);
    g_assert_no_error(error);
    g_assert_nonnull(record);
    g_ptr_array_add(records, record);

    g_assert_true(goree_terminal_remote_targets_save(records, &error));
    g_assert_no_error(error);

    struct stat file_stat;
    g_assert_cmpint(g_stat(test_path, &file_stat), ==, 0);
    g_assert_cmpuint(file_stat.st_mode & 0777, ==, 0600);

    struct stat dir_stat;
    g_assert_cmpint(g_stat(test_directory, &dir_stat), ==, 0);
    g_assert_cmpuint(dir_stat.st_mode & 0777, ==, 0700);

    char *data = NULL;
    gsize length = 0;
    g_assert_true(g_file_get_contents(test_path, &data, &length, &error));
    g_assert_no_error(error);
    g_assert_nonnull(strstr(data, "name=Operations"));
    g_assert_nonnull(strstr(data, "host=admin.example.internal"));
    g_assert_nonnull(strstr(data, "username=operator"));
    g_assert_nonnull(strstr(data, "port=2222"));
    g_assert_null(strstr(data, "password"));
    g_assert_null(strstr(data, "token"));
    g_assert_null(strstr(data, "private-key"));
    g_assert_null(strstr(data, "IdentityFile"));
    g_assert_null(strstr(data, "ProxyCommand"));
    g_assert_null(strstr(data, "command="));
    g_assert_null(strstr(data, "StrictHostKeyChecking"));
    g_free(data);

    GPtrArray *loaded = goree_terminal_remote_targets_load(&error);
    g_assert_no_error(error);
    g_assert_nonnull(loaded);
    g_assert_cmpuint(loaded->len, ==, 1);
    const GoreeTerminalRemoteTargetRecord *loaded_record =
        goree_terminal_remote_targets_find(loaded, "ops");
    g_assert_nonnull(loaded_record);
    g_assert_cmpstr(loaded_record->name, ==, "Operations");
    g_assert_cmpstr(loaded_record->target.host, ==, "admin.example.internal");
    g_assert_cmpstr(loaded_record->target.username, ==, "operator");
    g_assert_cmpuint(loaded_record->target.port, ==, 2222);

    g_ptr_array_unref(loaded);
    g_ptr_array_unref(records);
}

static void
test_invalid_destination_rejected(void)
{
    GError *error = NULL;
    GoreeTerminalRemoteTargetRecord *record = goree_terminal_remote_target_record_new(
        "bad",
        "Bad",
        "-oProxyCommand=bad",
        "operator",
        22,
        &error);
    g_assert_null(record);
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
}

static void
test_duplicate_ids_rejected_on_save(void)
{
    reset_file();
    GError *error = NULL;
    GPtrArray *records = new_record_array();
    g_ptr_array_add(
        records,
        goree_terminal_remote_target_record_new(
            "same", "First", "one.example", "", 22, &error));
    g_assert_no_error(error);
    g_ptr_array_add(
        records,
        goree_terminal_remote_target_record_new(
            "same", "Second", "two.example", "", 22, &error));
    g_assert_no_error(error);

    g_assert_false(goree_terminal_remote_targets_save(records, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    g_ptr_array_unref(records);
}

static void
test_unknown_or_secret_field_rejected_on_load(void)
{
    reset_file();
    const char *bad =
        "[RemoteTarget:ops]\n"
        "name=Operations\n"
        "host=admin.example.internal\n"
        "username=operator\n"
        "port=22\n"
        "password=must-not-be-stored\n";
    GError *error = NULL;
    g_assert_true(g_file_set_contents(test_path, bad, -1, &error));
    g_assert_no_error(error);

    GPtrArray *records = goree_terminal_remote_targets_load(&error);
    g_assert_null(records);
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
}

static void
test_record_id_and_name_validation(void)
{
    GError *error = NULL;
    GoreeTerminalRemoteTargetRecord *record = goree_terminal_remote_target_record_new(
        "Invalid ID",
        "Operations",
        "admin.example",
        "",
        22,
        &error);
    g_assert_null(record);
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    record = goree_terminal_remote_target_record_new(
        "ops",
        "",
        "admin.example",
        "",
        22,
        &error);
    g_assert_null(record);
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    test_directory = g_dir_make_tmp("goreecloud-terminal-remote-targets-XXXXXX", NULL);
    g_assert_nonnull(test_directory);
    test_path = g_build_filename(test_directory, "remote-targets.ini", NULL);
    g_setenv("GOREE_TERMINAL_REMOTE_TARGETS_PATH", test_path, TRUE);

    g_test_add_func("/remote-target-store/missing", test_missing_file_returns_empty);
    g_test_add_func("/remote-target-store/round-trip", test_round_trip_private_metadata_only);
    g_test_add_func("/remote-target-store/invalid-destination", test_invalid_destination_rejected);
    g_test_add_func("/remote-target-store/duplicate-id", test_duplicate_ids_rejected_on_save);
    g_test_add_func("/remote-target-store/secret-field", test_unknown_or_secret_field_rejected_on_load);
    g_test_add_func("/remote-target-store/id-name", test_record_id_and_name_validation);

    int status = g_test_run();
    reset_file();
    g_rmdir(test_directory);
    g_free(test_path);
    g_free(test_directory);
    return status;
}
