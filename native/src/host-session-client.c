#define _GNU_SOURCE

#include "host-session-client.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static gboolean
set_errno_error(GError **error, int error_number, const char *message)
{
    g_set_error(error,
                G_IO_ERROR,
                g_io_error_from_errno(error_number),
                "%s: %s",
                message,
                g_strerror(error_number));
    return FALSE;
}

static char *
build_runtime_directory(void)
{
    const char *runtime = g_get_user_runtime_dir();

    if (runtime == NULL || *runtime == '\0')
        return NULL;

    return g_build_filename(runtime, GOREE_TERMINAL_HOST_RUNTIME_DIR, NULL);
}

static char *
build_socket_path(void)
{
    char *runtime_directory = build_runtime_directory();
    char *socket_path;

    if (runtime_directory == NULL)
        return NULL;

    socket_path = g_build_filename(runtime_directory,
                                   GOREE_TERMINAL_HOST_SOCKET_NAME,
                                   NULL);
    g_free(runtime_directory);
    return socket_path;
}

static gboolean
validate_socket_boundary(const char *socket_path, GError **error)
{
    struct stat socket_stat;
    char *runtime_directory = g_path_get_dirname(socket_path);
    struct stat directory_stat;
    uid_t uid = getuid();

    if (lstat(runtime_directory, &directory_stat) != 0) {
        int saved_errno = errno;
        g_free(runtime_directory);
        return set_errno_error(error,
                               saved_errno,
                               "Host-session runtime directory is unavailable");
    }

    if (!S_ISDIR(directory_stat.st_mode) || directory_stat.st_uid != uid ||
        (directory_stat.st_mode & 0777) != 0700) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_PERMISSION_DENIED,
                            "Host-session runtime directory failed ownership or mode validation");
        g_free(runtime_directory);
        return FALSE;
    }
    g_free(runtime_directory);

    if (lstat(socket_path, &socket_stat) != 0)
        return set_errno_error(error,
                               errno,
                               "Host-session socket is unavailable");

    if (!S_ISSOCK(socket_stat.st_mode) || socket_stat.st_uid != uid ||
        (socket_stat.st_mode & 0777) != 0600) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_PERMISSION_DENIED,
                            "Host-session socket failed ownership or mode validation");
        return FALSE;
    }

    return TRUE;
}

static gboolean
write_full(int fd, const void *buffer, gsize length, GError **error)
{
    const guint8 *bytes = buffer;
    gsize offset = 0;

    while (offset < length) {
        ssize_t written = send(fd,
                               bytes + offset,
                               length - offset,
                               MSG_NOSIGNAL);
        if (written < 0) {
            if (errno == EINTR)
                continue;
            return set_errno_error(error, errno, "Host-session request failed");
        }
        if (written == 0) {
            g_set_error_literal(error,
                                G_IO_ERROR,
                                G_IO_ERROR_BROKEN_PIPE,
                                "Host-session request connection closed unexpectedly");
            return FALSE;
        }
        offset += (gsize) written;
    }

    return TRUE;
}

static gboolean
validate_message(const GoreeTerminalHostMessage *message, GError **error)
{
    if (message->magic != GOREE_TERMINAL_HOST_PROTOCOL_MAGIC ||
        message->version != GOREE_TERMINAL_HOST_PROTOCOL_VERSION) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_INVALID_DATA,
                            "Host-session protocol identity or version is invalid");
        return FALSE;
    }

    return TRUE;
}

static gboolean
receive_spawn_response(int fd,
                       GoreeTerminalHostMessage *response,
                       int *pty_fd,
                       GError **error)
{
    struct iovec iov = {
        .iov_base = response,
        .iov_len = sizeof(*response),
    };
    guint8 control[CMSG_SPACE(sizeof(int))];
    struct msghdr message;
    ssize_t received;
    int flags = MSG_WAITALL;

#ifdef MSG_CMSG_CLOEXEC
    flags |= MSG_CMSG_CLOEXEC;
#endif

    memset(control, 0, sizeof(control));
    memset(&message, 0, sizeof(message));
    message.msg_iov = &iov;
    message.msg_iovlen = 1;
    message.msg_control = control;
    message.msg_controllen = sizeof(control);

    do {
        received = recvmsg(fd, &message, flags);
    } while (received < 0 && errno == EINTR);

    if (received < 0)
        return set_errno_error(error, errno, "Host-session response failed");
    if (received != (ssize_t) sizeof(*response)) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_INVALID_DATA,
                            "Host-session response was truncated");
        return FALSE;
    }
    if ((message.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) != 0) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_INVALID_DATA,
                            "Host-session response ancillary data was truncated");
        return FALSE;
    }
    if (!validate_message(response, error))
        return FALSE;

    if (response->type == GOREE_TERMINAL_HOST_MESSAGE_ERROR) {
        int remote_error = response->value > 0 ? response->value : EIO;
        return set_errno_error(error, remote_error, "Host agent rejected the session");
    }
    if (response->type != GOREE_TERMINAL_HOST_MESSAGE_SPAWN_RESPONSE ||
        response->value <= 0) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_INVALID_DATA,
                            "Host-session response type or child identity is invalid");
        return FALSE;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&message);
    if (cmsg == NULL || cmsg->cmsg_level != SOL_SOCKET ||
        cmsg->cmsg_type != SCM_RIGHTS ||
        cmsg->cmsg_len != CMSG_LEN(sizeof(int)) ||
        CMSG_NXTHDR(&message, cmsg) != NULL) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_INVALID_DATA,
                            "Host-session response did not contain exactly one PTY descriptor");
        return FALSE;
    }

    memcpy(pty_fd, CMSG_DATA(cmsg), sizeof(*pty_fd));
    if (*pty_fd < 0) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_INVALID_DATA,
                            "Host-session response contained an invalid PTY descriptor");
        return FALSE;
    }

#ifndef MSG_CMSG_CLOEXEC
    int descriptor_flags = fcntl(*pty_fd, F_GETFD);
    if (descriptor_flags < 0 ||
        fcntl(*pty_fd, F_SETFD, descriptor_flags | FD_CLOEXEC) != 0) {
        int saved_errno = errno;
        close(*pty_fd);
        *pty_fd = -1;
        return set_errno_error(error,
                               saved_errno,
                               "Unable to secure host-session PTY descriptor");
    }
#endif

    return TRUE;
}

static gboolean
verify_peer_uid(int fd, GError **error)
{
#ifdef SO_PEERCRED
    struct ucred credentials;
    socklen_t length = sizeof(credentials);

    memset(&credentials, 0, sizeof(credentials));
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0)
        return set_errno_error(error,
                               errno,
                               "Unable to verify host-agent peer credentials");
    if (length != sizeof(credentials) || credentials.uid != getuid()) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_PERMISSION_DENIED,
                            "Host-agent peer UID does not match the current user");
        return FALSE;
    }
    return TRUE;
#else
    (void) fd;
    g_set_error_literal(error,
                        G_IO_ERROR,
                        G_IO_ERROR_NOT_SUPPORTED,
                        "Host-agent peer credential verification is unavailable");
    return FALSE;
#endif
}

void
goree_terminal_host_session_init(GoreeTerminalHostSession *session)
{
    g_return_if_fail(session != NULL);

    memset(session, 0, sizeof(*session));
    session->control_fd = -1;
    session->pty_fd = -1;
    session->child_pid = -1;
}

gboolean
goree_terminal_host_session_connect(GoreeTerminalHostSession *session,
                                    guint rows,
                                    guint columns,
                                    GError **error)
{
    char *socket_path = NULL;
    int fd = -1;
    struct sockaddr_un address;
    GoreeTerminalHostMessage request = {
        .magic = GOREE_TERMINAL_HOST_PROTOCOL_MAGIC,
        .version = GOREE_TERMINAL_HOST_PROTOCOL_VERSION,
        .type = GOREE_TERMINAL_HOST_MESSAGE_SPAWN_REQUEST,
        .value = 0,
        .rows = rows,
        .columns = columns,
    };
    GoreeTerminalHostMessage response;
    int pty_fd = -1;

    g_return_val_if_fail(session != NULL, FALSE);
    g_return_val_if_fail(session->control_fd < 0 && session->pty_fd < 0, FALSE);

    socket_path = build_socket_path();
    if (socket_path == NULL) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_NOT_FOUND,
                            "No user runtime directory is available for the host-session socket");
        return FALSE;
    }
    if (!validate_socket_boundary(socket_path, error)) {
        g_free(socket_path);
        return FALSE;
    }

    if (strlen(socket_path) >= sizeof(address.sun_path)) {
        g_set_error_literal(error,
                            G_IO_ERROR,
                            G_IO_ERROR_FILENAME_TOO_LONG,
                            "Host-session socket path is too long");
        g_free(socket_path);
        return FALSE;
    }

    fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        int saved_errno = errno;
        g_free(socket_path);
        return set_errno_error(error,
                               saved_errno,
                               "Unable to create host-session socket");
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    memcpy(address.sun_path, socket_path, strlen(socket_path) + 1);
    g_free(socket_path);

    if (connect(fd, (struct sockaddr *) &address, sizeof(address)) != 0) {
        int saved_errno = errno;
        close(fd);
        return set_errno_error(error,
                               saved_errno,
                               "Unable to connect to the GoreeCloud Terminal host agent");
    }
    if (!verify_peer_uid(fd, error)) {
        close(fd);
        return FALSE;
    }
    if (!write_full(fd, &request, sizeof(request), error)) {
        close(fd);
        return FALSE;
    }

    memset(&response, 0, sizeof(response));
    if (!receive_spawn_response(fd, &response, &pty_fd, error)) {
        close(fd);
        return FALSE;
    }

    int status_flags = fcntl(fd, F_GETFL);
    if (status_flags < 0 || fcntl(fd, F_SETFL, status_flags | O_NONBLOCK) != 0) {
        int saved_errno = errno;
        close(pty_fd);
        close(fd);
        return set_errno_error(error,
                               saved_errno,
                               "Unable to configure host-session control channel");
    }

    session->control_fd = fd;
    session->pty_fd = pty_fd;
    session->child_pid = (pid_t) response.value;
    session->event_offset = 0;
    return TRUE;
}

int
goree_terminal_host_session_steal_pty_fd(GoreeTerminalHostSession *session)
{
    int fd;

    g_return_val_if_fail(session != NULL, -1);

    fd = session->pty_fd;
    session->pty_fd = -1;
    return fd;
}

int
goree_terminal_host_session_control_fd(const GoreeTerminalHostSession *session)
{
    g_return_val_if_fail(session != NULL, -1);
    return session->control_fd;
}

pid_t
goree_terminal_host_session_child_pid(const GoreeTerminalHostSession *session)
{
    g_return_val_if_fail(session != NULL, -1);
    return session->child_pid;
}

GoreeTerminalHostEvent
goree_terminal_host_session_poll_event(GoreeTerminalHostSession *session,
                                       int *value,
                                       GError **error)
{
    GoreeTerminalHostMessage message;

    g_return_val_if_fail(session != NULL, GOREE_TERMINAL_HOST_EVENT_ERROR);
    if (value != NULL)
        *value = 0;

    if (session->control_fd < 0)
        return GOREE_TERMINAL_HOST_EVENT_DISCONNECTED;

    while (session->event_offset < sizeof(message)) {
        ssize_t received = recv(session->control_fd,
                                session->event_buffer + session->event_offset,
                                sizeof(message) - session->event_offset,
                                MSG_DONTWAIT);
        if (received > 0) {
            session->event_offset += (gsize) received;
            continue;
        }
        if (received == 0) {
            if (session->event_offset != 0) {
                g_set_error_literal(error,
                                    G_IO_ERROR,
                                    G_IO_ERROR_INVALID_DATA,
                                    "Host-session control channel closed mid-message");
                session->event_offset = 0;
                return GOREE_TERMINAL_HOST_EVENT_ERROR;
            }
            return GOREE_TERMINAL_HOST_EVENT_DISCONNECTED;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return GOREE_TERMINAL_HOST_EVENT_NONE;
        set_errno_error(error, errno, "Host-session control channel failed");
        session->event_offset = 0;
        return GOREE_TERMINAL_HOST_EVENT_ERROR;
    }

    memcpy(&message, session->event_buffer, sizeof(message));
    session->event_offset = 0;

    if (!validate_message(&message, error))
        return GOREE_TERMINAL_HOST_EVENT_ERROR;

    if (value != NULL)
        *value = message.value;

    if (message.type == GOREE_TERMINAL_HOST_MESSAGE_EXIT)
        return GOREE_TERMINAL_HOST_EVENT_EXITED;
    if (message.type == GOREE_TERMINAL_HOST_MESSAGE_ERROR)
        return GOREE_TERMINAL_HOST_EVENT_ERROR;

    g_set_error_literal(error,
                        G_IO_ERROR,
                        G_IO_ERROR_INVALID_DATA,
                        "Host-session control channel returned an unexpected message type");
    return GOREE_TERMINAL_HOST_EVENT_ERROR;
}

void
goree_terminal_host_session_close(GoreeTerminalHostSession *session)
{
    g_return_if_fail(session != NULL);

    if (session->pty_fd >= 0)
        close(session->pty_fd);
    if (session->control_fd >= 0)
        close(session->control_fd);

    goree_terminal_host_session_init(session);
}
