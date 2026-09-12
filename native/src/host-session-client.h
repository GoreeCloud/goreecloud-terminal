#pragma once

#include <gio/gio.h>
#include <glib.h>
#include <sys/types.h>

#include "host-session-protocol.h"

typedef enum {
    GOREE_TERMINAL_HOST_EVENT_NONE = 0,
    GOREE_TERMINAL_HOST_EVENT_EXITED,
    GOREE_TERMINAL_HOST_EVENT_ERROR,
    GOREE_TERMINAL_HOST_EVENT_DISCONNECTED,
} GoreeTerminalHostEvent;

typedef struct {
    int control_fd;
    int pty_fd;
    pid_t child_pid;
    guint8 event_buffer[sizeof(GoreeTerminalHostMessage)];
    gsize event_offset;
} GoreeTerminalHostSession;

void goree_terminal_host_session_init(GoreeTerminalHostSession *session);

/*
 * Connect to the same-user GoreeCloud Terminal host agent and request the
 * authenticated user's default host shell. No command, terminal contents,
 * credentials, environment dump, or working-directory history crosses this
 * control protocol.
 */
gboolean goree_terminal_host_session_connect(
    GoreeTerminalHostSession *session,
    guint rows,
    guint columns,
    GError **error);

/* Transfer ownership of the received PTY master to VTE. */
int goree_terminal_host_session_steal_pty_fd(
    GoreeTerminalHostSession *session);

int goree_terminal_host_session_control_fd(
    const GoreeTerminalHostSession *session);

pid_t goree_terminal_host_session_child_pid(
    const GoreeTerminalHostSession *session);

/*
 * Read at most one complete control event without blocking. The returned value
 * is the raw waitpid(2) status for EXITED or an errno-style value for ERROR.
 */
GoreeTerminalHostEvent goree_terminal_host_session_poll_event(
    GoreeTerminalHostSession *session,
    int *value,
    GError **error);

void goree_terminal_host_session_close(GoreeTerminalHostSession *session);
