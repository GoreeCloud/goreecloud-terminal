#include <glib.h>
#include <string.h>

#include "remote-openssh.h"
#include "remote-session.h"

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

static GoreeTerminalRemotePrivacyEvidence
accepted_evidence(const GoreeTerminalRemoteTarget *target)
{
    GoreeTerminalRemotePrivacyEvidence evidence = {0};
    GError *error = NULL;
    evidence.decision = GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW;
    evidence.authority_verified = TRUE;
    evidence.runtime_accepted = TRUE;
    evidence.expires_at_unix = TEST_NOW + 300;
    g_strlcpy(evidence.purpose, GOREE_TERMINAL_REMOTE_PURPOSE, sizeof(evidence.purpose));
    g_assert_true(goree_terminal_remote_target_binding(
        target,
        evidence.target_binding,
        &error));
    g_assert_no_error(error);
    return evidence;
}

static void
test_target_validation(void)
{
    GoreeTerminalRemoteTarget target;
    GError *error = NULL;

    g_assert_true(goree_terminal_remote_target_init(
        &target,
        "host.example",
        "admin_user",
        0,
        &error));
    g_assert_no_error(error);
    g_assert_cmpstr(target.host, ==, "host.example");
    g_assert_cmpstr(target.username, ==, "admin_user");
    g_assert_cmpuint(target.port, ==, 22);

    g_assert_false(goree_terminal_remote_target_init(
        &target,
        "-oProxyCommand=bad",
        "admin",
        22,
        &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    g_assert_false(goree_terminal_remote_target_init(
        &target,
        "host.example;command",
        "admin",
        22,
        &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    g_assert_false(goree_terminal_remote_target_init(
        &target,
        "host.example",
        "admin@other",
        22,
        &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    g_assert_false(goree_terminal_remote_target_init(
        &target,
        "host.example",
        "admin",
        70000,
        &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
}

static void
test_target_binding_is_exact_and_deterministic(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    char first[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX];
    char second[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX];
    GError *error = NULL;

    g_assert_true(goree_terminal_remote_target_binding(&target, first, &error));
    g_assert_no_error(error);
    g_assert_true(g_str_has_prefix(first, "sha256:"));
    g_assert_true(goree_terminal_remote_target_binding(&target, second, &error));
    g_assert_no_error(error);
    g_assert_cmpstr(first, ==, second);

    target.port = 2222;
    g_assert_true(goree_terminal_remote_target_binding(&target, second, &error));
    g_assert_no_error(error);
    g_assert_cmpstr(first, !=, second);
}

static void
test_privacy_evidence_fails_closed(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemotePrivacyEvidence evidence = accepted_evidence(&target);
    GError *error = NULL;

    g_assert_true(goree_terminal_remote_privacy_evidence_validate(
        &target,
        &evidence,
        TEST_NOW,
        &error));
    g_assert_no_error(error);

    evidence.authority_verified = FALSE;
    g_assert_false(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    evidence.authority_verified = TRUE;

    evidence.runtime_accepted = FALSE;
    g_assert_false(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    evidence.runtime_accepted = TRUE;

    evidence.decision = GOREE_TERMINAL_REMOTE_PRIVACY_DENY;
    g_assert_false(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    evidence.decision = GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW;

    evidence.expires_at_unix = TEST_NOW;
    g_assert_false(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
    evidence.expires_at_unix = TEST_NOW + 300;

    g_strlcpy(evidence.target_binding, "sha256:not-the-target", sizeof(evidence.target_binding));
    g_assert_false(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);
}

static void
test_privacy_constraints_fail_closed(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemotePrivacyEvidence evidence = accepted_evidence(&target);
    GError *error = NULL;

    evidence.decision = GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW_WITH_CONSTRAINTS;
    evidence.constraints_present = TRUE;
    evidence.constraints_satisfied = FALSE;
    g_assert_false(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE);
    g_clear_error(&error);

    evidence.constraints_satisfied = TRUE;
    g_assert_true(goree_terminal_remote_privacy_evidence_validate(
        &target, &evidence, TEST_NOW, &error));
    g_assert_no_error(error);
}

static void
test_remote_lifecycle_and_reauthorization(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemotePrivacyEvidence evidence = accepted_evidence(&target);
    GoreeTerminalRemoteSecurityContext security = {
        .coverage = GOREE_TERMINAL_REMOTE_COVERAGE_NOT_COVERED,
        .evidence_valid = TRUE,
        .runtime_accepted = FALSE,
    };
    GoreeTerminalRemoteSession session;

    goree_terminal_remote_session_init(
        &session, &target, &evidence, &security, TEST_NOW);
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_READY);
    g_assert_true(goree_terminal_remote_session_can_connect(&session, TEST_NOW));
    g_assert_true(goree_terminal_remote_session_begin_connect(&session, TEST_NOW));
    g_assert_true(goree_terminal_remote_session_mark_host_key_verification(&session));
    g_assert_true(goree_terminal_remote_session_mark_authenticating(&session));
    g_assert_true(goree_terminal_remote_session_mark_running(&session));

    goree_terminal_remote_session_mark_disconnected(&session);
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_DISCONNECTED);

    GoreeTerminalRemotePrivacyEvidence expired = evidence;
    expired.expires_at_unix = TEST_NOW;
    g_assert_false(goree_terminal_remote_session_begin_reconnect(
        &session, &expired, TEST_NOW));
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_BLOCKED);

    goree_terminal_remote_session_init(
        &session, &target, &evidence, &security, TEST_NOW);
    g_assert_true(goree_terminal_remote_session_begin_connect(&session, TEST_NOW));
    goree_terminal_remote_session_mark_disconnected(&session);
    GoreeTerminalRemotePrivacyEvidence refreshed = evidence;
    refreshed.expires_at_unix = TEST_NOW + 600;
    g_assert_true(goree_terminal_remote_session_begin_reconnect(
        &session, &refreshed, TEST_NOW));
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_RECONNECTING);
}

static void
test_wardveil_coverage_does_not_create_authority(void)
{
    GoreeTerminalRemoteTarget target = valid_target();
    GoreeTerminalRemotePrivacyEvidence denied = accepted_evidence(&target);
    denied.decision = GOREE_TERMINAL_REMOTE_PRIVACY_DENY;
    GoreeTerminalRemoteSecurityContext security = {
        .coverage = GOREE_TERMINAL_REMOTE_COVERAGE_COVERED,
        .evidence_valid = TRUE,
        .runtime_accepted = TRUE,
    };
    GoreeTerminalRemoteSession session;

    goree_terminal_remote_session_init(
        &session, &target, &denied, &security, TEST_NOW);
    g_assert_cmpint(session.state, ==, GOREE_TERMINAL_REMOTE_BLOCKED);
    g_assert_false(goree_terminal_remote_session_can_connect(&session, TEST_NOW));
    g_assert_cmpstr(goree_terminal_remote_coverage_label(&security), ==, "Wardveil: Covered");
}

static void
test_openssh_argv_is_structured_and_interactive(void)
{
    GoreeTerminalRemoteTarget target;
    GError *error = NULL;
    g_assert_true(goree_terminal_remote_target_init(
        &target,
        "admin.example.internal",
        "operator",
        2222,
        &error));
    g_assert_no_error(error);

    char **argv = NULL;
    g_assert_true(goree_terminal_remote_openssh_build_argv(&target, &argv, &error));
    g_assert_no_error(error);
    g_assert_nonnull(argv);
    g_assert_cmpstr(argv[0], ==, "ssh");
    g_assert_cmpstr(argv[1], ==, "-p");
    g_assert_cmpstr(argv[2], ==, "2222");
    g_assert_cmpstr(argv[3], ==, "-l");
    g_assert_cmpstr(argv[4], ==, "operator");
    g_assert_cmpstr(argv[5], ==, "admin.example.internal");
    g_assert_null(argv[6]);

    for (guint i = 0; argv[i] != NULL; i++) {
        g_assert_null(strstr(argv[i], "StrictHostKeyChecking"));
        g_assert_null(strstr(argv[i], "ProxyCommand"));
        g_assert_null(strstr(argv[i], "password"));
        g_assert_null(strstr(argv[i], "IdentityFile"));
    }
    g_strfreev(argv);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/remote/target-validation", test_target_validation);
    g_test_add_func("/remote/target-binding", test_target_binding_is_exact_and_deterministic);
    g_test_add_func("/remote/privacy-fail-closed", test_privacy_evidence_fails_closed);
    g_test_add_func("/remote/privacy-constraints", test_privacy_constraints_fail_closed);
    g_test_add_func("/remote/lifecycle", test_remote_lifecycle_and_reauthorization);
    g_test_add_func("/remote/wardveil-authority-separation", test_wardveil_coverage_does_not_create_authority);
    g_test_add_func("/remote/openssh-argv", test_openssh_argv_is_structured_and_interactive);
    return g_test_run();
}
