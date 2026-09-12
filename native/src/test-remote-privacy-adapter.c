#include <glib.h>
#include <string.h>

#include "remote-privacy-adapter.h"

#define TEST_NOW G_GINT64_CONSTANT(1800000000)

static GoreeTerminalRemoteTarget
valid_target(void)
{
    GoreeTerminalRemoteTarget target;
    GError *error = NULL;
    g_assert_true(goree_terminal_remote_target_init(
        &target,
        "admin.example.internal",
        "operator",
        22,
        &error));
    g_assert_no_error(error);
    return target;
}

static void
test_default_adapter_is_unavailable_and_bound(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemotePrivacyAdapter adapter;
    GoreeTerminalRemotePrivacyEvidence evidence;
    GError *error = NULL;

    goree_terminal_remote_privacy_adapter_default(&adapter);
    g_assert_true(goree_terminal_remote_privacy_adapter_query(
        &adapter,
        &target,
        TEST_NOW,
        &evidence,
        &error));
    g_assert_no_error(error);
    g_assert_cmpint(evidence.decision, ==, GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE);
    g_assert_false(evidence.authority_verified);
    g_assert_false(evidence.runtime_accepted);
    g_assert_false(evidence.constraints_present);
    g_assert_false(evidence.constraints_satisfied);
    g_assert_cmpint(evidence.expires_at_unix, ==, 0);
    g_assert_cmpstr(evidence.purpose, ==, GOREE_TERMINAL_REMOTE_PURPOSE);
    g_assert_true(g_str_has_prefix(evidence.target_binding, "sha256:"));

    char expected[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX];
    g_assert_true(goree_terminal_remote_target_binding(&target, expected, &error));
    g_assert_no_error(error);
    g_assert_cmpstr(evidence.target_binding, ==, expected);
}

static void
test_null_adapter_uses_fail_closed_default(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemoteSession session;
    GError *error = NULL;

    g_assert_true(goree_terminal_remote_session_init_from_privacy_adapter(
        &session,
        &target,
        NULL,
        NULL,
        TEST_NOW,
        &error));
    g_assert_no_error(error);
    g_assert_cmpint(session.privacy.decision, ==, GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE);
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_BLOCKED);
    g_assert_false(goree_terminal_remote_session_can_connect(&session, TEST_NOW));
    g_assert_false(goree_terminal_remote_session_begin_connect(&session, TEST_NOW));
}

static gboolean
malformed_adapter(
    const GoreeTerminalRemoteTarget *target,
    gint64 now_unix,
    GoreeTerminalRemotePrivacyEvidence *evidence,
    gpointer user_data,
    GError **error)
{
    (void) target;
    (void) now_unix;
    (void) user_data;
    (void) error;
    memset(evidence, 0, sizeof(*evidence));
    evidence->decision = GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW;
    evidence->authority_verified = TRUE;
    evidence->runtime_accepted = TRUE;
    evidence->expires_at_unix = TEST_NOW + 100;
    return TRUE;
}

static void
test_adapter_cannot_omit_operation_binding(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemotePrivacyAdapter adapter = {
        .authorize = malformed_adapter,
        .user_data = NULL,
    };
    GoreeTerminalRemotePrivacyEvidence evidence;
    GError *error = NULL;

    g_assert_false(goree_terminal_remote_privacy_adapter_query(
        &adapter,
        &target,
        TEST_NOW,
        &evidence,
        &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    g_assert_cmpint(evidence.decision, ==, GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE);
    g_assert_false(evidence.authority_verified);
    g_assert_false(evidence.runtime_accepted);
}

static void
test_default_adapter_cannot_become_authority_by_security_coverage(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemoteSecurityContext security = {
        .coverage = GOREE_TERMINAL_REMOTE_COVERAGE_COVERED,
        .evidence_valid = TRUE,
        .runtime_accepted = TRUE,
    };
    GoreeTerminalRemoteSession session;
    GError *error = NULL;

    g_assert_true(goree_terminal_remote_session_init_from_privacy_adapter(
        &session,
        &target,
        NULL,
        &security,
        TEST_NOW,
        &error));
    g_assert_no_error(error);
    g_assert_cmpstr(goree_terminal_remote_coverage_label(&session.security), ==, "Wardveil: Covered");
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_BLOCKED);
    g_assert_false(goree_terminal_remote_session_can_connect(&session, TEST_NOW));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/remote-privacy/default-unavailable", test_default_adapter_is_unavailable_and_bound);
    g_test_add_func("/remote-privacy/null-adapter", test_null_adapter_uses_fail_closed_default);
    g_test_add_func("/remote-privacy/operation-binding", test_adapter_cannot_omit_operation_binding);
    g_test_add_func("/remote-privacy/coverage-separation", test_default_adapter_cannot_become_authority_by_security_coverage);
    return g_test_run();
}
