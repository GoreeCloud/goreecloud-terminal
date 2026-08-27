#include "session-lifecycle.h"

void
goree_terminal_session_lifecycle_init(
    GoreeTerminalSessionLifecycle *lifecycle,
    guint session_id)
{
    g_return_if_fail(lifecycle != NULL);
    g_return_if_fail(session_id > 0);

    lifecycle->id = session_id;
    lifecycle->state = GOREE_TERMINAL_SESSION_STARTING;
    lifecycle->has_exit_status = FALSE;
    lifecycle->exit_status = 0;
}

gboolean
goree_terminal_session_mark_running(GoreeTerminalSessionLifecycle *lifecycle)
{
    g_return_val_if_fail(lifecycle != NULL, FALSE);

    if (lifecycle->state != GOREE_TERMINAL_SESSION_STARTING)
        return FALSE;

    lifecycle->state = GOREE_TERMINAL_SESSION_RUNNING;
    return TRUE;
}

void
goree_terminal_session_mark_child_exited(
    GoreeTerminalSessionLifecycle *lifecycle,
    int exit_status)
{
    g_return_if_fail(lifecycle != NULL);

    lifecycle->has_exit_status = TRUE;
    lifecycle->exit_status = exit_status;

    if (lifecycle->state != GOREE_TERMINAL_SESSION_CLOSING)
        lifecycle->state = GOREE_TERMINAL_SESSION_EXITED;
}

void
goree_terminal_session_request_close(GoreeTerminalSessionLifecycle *lifecycle)
{
    g_return_if_fail(lifecycle != NULL);
    lifecycle->state = GOREE_TERMINAL_SESSION_CLOSING;
}

gboolean
goree_terminal_session_can_accept_input(
    const GoreeTerminalSessionLifecycle *lifecycle)
{
    g_return_val_if_fail(lifecycle != NULL, FALSE);
    return lifecycle->state == GOREE_TERMINAL_SESSION_RUNNING;
}

gboolean
goree_terminal_session_preserves_output(
    const GoreeTerminalSessionLifecycle *lifecycle)
{
    g_return_val_if_fail(lifecycle != NULL, FALSE);
    return lifecycle->state == GOREE_TERMINAL_SESSION_EXITED;
}
