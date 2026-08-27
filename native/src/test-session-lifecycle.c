#include <glib.h>

#include "session-lifecycle.h"

static void
test_start_running_exit(void)
{
    GoreeTerminalSessionLifecycle lifecycle;

    goree_terminal_session_lifecycle_init(&lifecycle, 7);
    g_assert_cmpuint(lifecycle.id, ==, 7);
    g_assert_cmpint(lifecycle.state, ==, GOREE_TERMINAL_SESSION_STARTING);
    g_assert_false(lifecycle.has_exit_status);
    g_assert_false(goree_terminal_session_can_accept_input(&lifecycle));

    g_assert_true(goree_terminal_session_mark_running(&lifecycle));
    g_assert_true(goree_terminal_session_can_accept_input(&lifecycle));

    goree_terminal_session_mark_child_exited(&lifecycle, 0);
    g_assert_cmpint(lifecycle.state, ==, GOREE_TERMINAL_SESSION_EXITED);
    g_assert_true(lifecycle.has_exit_status);
    g_assert_cmpint(lifecycle.exit_status, ==, 0);
    g_assert_false(goree_terminal_session_can_accept_input(&lifecycle));
    g_assert_true(goree_terminal_session_preserves_output(&lifecycle));
}

static void
test_close_running_session(void)
{
    GoreeTerminalSessionLifecycle lifecycle;

    goree_terminal_session_lifecycle_init(&lifecycle, 1);
    g_assert_true(goree_terminal_session_mark_running(&lifecycle));
    goree_terminal_session_request_close(&lifecycle);
    g_assert_cmpint(lifecycle.state, ==, GOREE_TERMINAL_SESSION_CLOSING);
    g_assert_false(goree_terminal_session_can_accept_input(&lifecycle));
    g_assert_false(goree_terminal_session_preserves_output(&lifecycle));
}

static void
test_child_exit_does_not_reopen_closing_session(void)
{
    GoreeTerminalSessionLifecycle lifecycle;

    goree_terminal_session_lifecycle_init(&lifecycle, 2);
    g_assert_true(goree_terminal_session_mark_running(&lifecycle));
    goree_terminal_session_request_close(&lifecycle);
    goree_terminal_session_mark_child_exited(&lifecycle, 9);

    g_assert_cmpint(lifecycle.state, ==, GOREE_TERMINAL_SESSION_CLOSING);
    g_assert_true(lifecycle.has_exit_status);
    g_assert_cmpint(lifecycle.exit_status, ==, 9);
}

static void
test_running_transition_is_one_way(void)
{
    GoreeTerminalSessionLifecycle lifecycle;

    goree_terminal_session_lifecycle_init(&lifecycle, 3);
    g_assert_true(goree_terminal_session_mark_running(&lifecycle));
    g_assert_false(goree_terminal_session_mark_running(&lifecycle));
    goree_terminal_session_mark_child_exited(&lifecycle, 1);
    g_assert_false(goree_terminal_session_mark_running(&lifecycle));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/native-session/start-running-exit", test_start_running_exit);
    g_test_add_func("/native-session/close-running", test_close_running_session);
    g_test_add_func(
        "/native-session/closing-child-exit",
        test_child_exit_does_not_reopen_closing_session);
    g_test_add_func("/native-session/running-one-way", test_running_transition_is_one_way);
    return g_test_run();
}
