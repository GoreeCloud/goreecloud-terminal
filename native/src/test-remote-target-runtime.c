#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>

#include "remote-target-runtime.h"

#define TEST_NOW G_GINT64_CONSTANT(1800000000)

static char *test_directory = NULL;
static char *test_path = NULL;

static void
reset_store(void)
{
    if (test_path != NULL)
        g_remove(test_path);
}

static GoreeTerminalRemoteTargetRecord *
make_record(const char *id, const char *host)
{
    GError *error = NULL;
    GoreeTerminalRemoteTargetRecord *record =
        goree_terminal_remote_target_record_new(
            id,
            "Operations",
            host,
            "operator",
            22,
            &error);
    g_assert_no_error(error);
    g_assert_nonnull(record);
    return record;
}

static void
test_missing_store_is_empty(void)
{
    reset_store();
    GError *error = NULL;
    GPtrArray *catalog = goree_terminal_remote_target_catalog_load(
        NULL,
        NULL,
        TEST_NOW,
        &error);
    g_assert_no_error(error);
    g_assert_nonnull(catalog);
    g_assert_cmpuint(catalog->len, ==, 0);
    g_ptr_array_unref(catalog);
}

static void
test_default_adapter_produces_visible_blocked_status(void)
{
    reset_store();
    GPtrArray *records = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_remote_target_record_free);
    g_ptr_array_add(records, make_record("ops", "admin.example.internal"));

    GError *error = NULL;
    g_assert_true(goree_terminal_remote_targets_save(records, &error));
    g_assert_no_error(error);
    g_ptr_array_unref(records);

    GPtrArray *catalog = goree_terminal_remote_target_catalog_load(
        NULL,
        NULL,
        TEST_NOW,
        &error);
    g_assert_no_error(error);
    g_assert_nonnull(catalog);
    g_assert_cmpuint(catalog->len, ==, 1);

    const GoreeTerminalRemoteTargetStatus *status =
        goree_terminal_remote_target_catalog_find(catalog, "ops");
    g_assert_nonnull(status);
    g_assert_cmpint(status->session.state, ==, GOREE_TERMINAL_REMOTE_BLOCKED);
    g_assert_cmpint(
        status->session.privacy.decision,
        ==,
        GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE);
    g_assert_cmpstr(
        goree_terminal_remote_target_status_label(status),
        ==,
        "Blocked — Privacy Shield unavailable");
    g_assert_false(goree_terminal_remote_session_can_connect(
        &status->session,
        TEST_NOW));

    g_ptr_array_unref(catalog);
}

static gboolean
accepted_adapter(
    const GoreeTerminalRemoteTarget *target,
    gint64 now_unix,
    GoreeTerminalRemotePrivacyEvidence *evidence,
    gpointer user_data,
    GError **error)
{
    (void) user_data;
    memset(evidence, 0, sizeof(*evidence));
    evidence->decision = GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW;
    evidence->authority_verified = TRUE;
    evidence->runtime_accepted = TRUE;
    evidence->expires_at_unix = now_unix + 300;
    g_strlcpy(
        evidence->purpose,
        GOREE_TERMINAL_REMOTE_PURPOSE,
        sizeof(evidence->purpose));
    return goree_terminal_remote_target_binding(
        target,
        evidence->target_binding,
        error);
}

static void
test_accepted_adapter_can_prepare_ready_status_without_connecting(void)
{
    GoreeTerminalRemoteTargetRecord *record = make_record(
        "ops",
        "admin.example.internal");
    GoreeTerminalRemotePrivacyAdapter adapter = {
        .authorize = accepted_adapter,
        .user_data = NULL,
    };
    GError *error = NULL;

    GoreeTerminalRemoteTargetStatus *status =
        goree_terminal_remote_target_status_new(
            record,
            &adapter,
            NULL,
            TEST_NOW,
            &error);
    g_assert_no_error(error);
    g_assert_nonnull(status);
    g_assert_cmpint(status->session.state, ==, GOREE_TERMINAL_REMOTE_READY);
    g_assert_true(goree_terminal_remote_session_can_connect(
        &status->session,
        TEST_NOW));
    g_assert_cmpstr(
        goree_terminal_remote_target_status_label(status),
        ==,
        "Ready — authorized");

    /* Preparing status never begins a network connection. */
    g_assert_cmpint(status->session.state, ==, GOREE_TERMINAL_REMOTE_READY);

    goree_terminal_remote_target_status_free(status);
    goree_terminal_remote_target_record_free(record);
}

static void
test_find_does_not_fallback_to_another_target(void)
{
    reset_store();
    GPtrArray *records = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_remote_target_record_free);
    g_ptr_array_add(records, make_record("first", "one.example.internal"));
    g_ptr_array_add(records, make_record("second", "two.example.internal"));

    GError *error = NULL;
    g_assert_true(goree_terminal_remote_targets_save(records, &error));
    g_assert_no_error(error);
    g_ptr_array_unref(records);

    GPtrArray *catalog = goree_terminal_remote_target_catalog_load(
        NULL,
        NULL,
        TEST_NOW,
        &error);
    g_assert_no_error(error);
    g_assert_nonnull(catalog);
    g_assert_nonnull(goree_terminal_remote_target_catalog_find(catalog, "second"));
    g_assert_null(goree_terminal_remote_target_catalog_find(catalog, "missing"));
    g_ptr_array_unref(catalog);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    test_directory = g_dir_make_tmp("goreecloud-terminal-remote-runtime-XXXXXX", NULL);
    g_assert_nonnull(test_directory);
    test_path = g_build_filename(test_directory, "remote-targets.ini", NULL);
    g_setenv("GOREE_TERMINAL_REMOTE_TARGETS_PATH", test_path, TRUE);

    g_test_add_func("/remote-target-runtime/missing", test_missing_store_is_empty);
    g_test_add_func("/remote-target-runtime/default-blocked", test_default_adapter_produces_visible_blocked_status);
    g_test_add_func("/remote-target-runtime/accepted-ready", test_accepted_adapter_can_prepare_ready_status_without_connecting);
    g_test_add_func("/remote-target-runtime/find", test_find_does_not_fallback_to_another_target);

    int status = g_test_run();
    reset_store();
    g_rmdir(test_directory);
    g_free(test_path);
    g_free(test_directory);
    return status;
}
