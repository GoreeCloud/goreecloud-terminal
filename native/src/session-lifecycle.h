#pragma once

#include <glib.h>

typedef enum {
    GOREE_TERMINAL_SESSION_STARTING,
    GOREE_TERMINAL_SESSION_RUNNING,
    GOREE_TERMINAL_SESSION_EXITED,
    GOREE_TERMINAL_SESSION_CLOSING,
} GoreeTerminalSessionState;

typedef struct {
    guint id;
    GoreeTerminalSessionState state;
    gboolean has_exit_status;
    int exit_status;
} GoreeTerminalSessionLifecycle;

void goree_terminal_session_lifecycle_init(
    GoreeTerminalSessionLifecycle *lifecycle,
    guint session_id);

gboolean goree_terminal_session_mark_running(
    GoreeTerminalSessionLifecycle *lifecycle);

void goree_terminal_session_mark_child_exited(
    GoreeTerminalSessionLifecycle *lifecycle,
    int exit_status);

void goree_terminal_session_request_close(
    GoreeTerminalSessionLifecycle *lifecycle);

gboolean goree_terminal_session_can_accept_input(
    const GoreeTerminalSessionLifecycle *lifecycle);

gboolean goree_terminal_session_preserves_output(
    const GoreeTerminalSessionLifecycle *lifecycle);
