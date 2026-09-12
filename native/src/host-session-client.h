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
    const char *shell_path;
    const char *working_directory;
    GoreeTerminalHostEnvironmentPolicy environment_policy;
    const char *const *environment_names;
    gsize environment_count;
} GoreeTerminalHostLaunchContext;

typedef struct {
    int control_fd;
    int pty_fd;
    pid_t child_pid;
    guint8 event_buffer[sizeof(GoreeTerminalHostMessage)];
    gsize event_offset;
} GoreeTerminalHostSession;

void goree_terminal_host_session_init(GoreeTerminalHostSession *session);

/*
 * Connect to the same-user GoreeCloud Terminal host agent using the default
 * authenticated-user launch context. This compatibility wrapper never sends a
 * command string or arbitrary execution payload.
 */
gboolean goree_terminal_host_session_connect(
    GoreeTerminalHostSession *session,
    guint rows,
    guint columns,
    GError **error);

/*
 * Connect with a bounded interactive-shell launch context. The client sends
 * only a validated shell path, absolute working directory, environment policy,
 * and environment variable names. Environment values, terminal contents,
 * credentials, tokens, private keys, shell history, and command strings are
 * excluded. The host agent independently validates every supplied field before
 * spawning the shell.
 */
gboolean goree_terminal_host_session_connect_with_context(
    GoreeTerminalHostSession *session,
    guint rows,
    guint columns,
    const GoreeTerminalHostLaunchContext *context,
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
