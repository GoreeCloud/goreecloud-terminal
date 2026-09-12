#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "host-session-protocol.h"

static char agent_socket_path[PATH_MAX];
static int agent_listen_fd = -1;

static const char *safe_environment_names[] = {
    "PATH",
    "LANG",
    "LC_ALL",
    "LC_CTYPE",
    "TZ",
    NULL,
};

static ssize_t
read_full(int fd, void *buffer, size_t length)
{
    size_t offset = 0;
    unsigned char *bytes = buffer;

    while (offset < length) {
        ssize_t nread = read(fd, bytes + offset, length - offset);
        if (nread == 0)
            return (ssize_t) offset;
        if (nread < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        offset += (size_t) nread;
    }

    return (ssize_t) offset;
}

static ssize_t
write_full(int fd, const void *buffer, size_t length)
{
    size_t offset = 0;
    const unsigned char *bytes = buffer;

    while (offset < length) {
        ssize_t nwritten = write(fd, bytes + offset, length - offset);
        if (nwritten < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        offset += (size_t) nwritten;
    }

    return (ssize_t) offset;
}

static GoreeTerminalHostMessage
make_message(GoreeTerminalHostMessageType type, int32_t value)
{
    GoreeTerminalHostMessage message = {
        .magic = GOREE_TERMINAL_HOST_PROTOCOL_MAGIC,
        .version = GOREE_TERMINAL_HOST_PROTOCOL_VERSION,
        .type = (uint16_t) type,
        .value = value,
        .rows = 0,
        .columns = 0,
    };
    return message;
}

static bool
send_message(int fd, GoreeTerminalHostMessageType type, int32_t value)
{
    GoreeTerminalHostMessage message = make_message(type, value);
    return write_full(fd, &message, sizeof(message)) == (ssize_t) sizeof(message);
}

static bool
send_message_with_fd(int socket_fd,
                     GoreeTerminalHostMessageType type,
                     int32_t value,
                     int passed_fd)
{
    GoreeTerminalHostMessage message = make_message(type, value);
    struct iovec iov = {
        .iov_base = &message,
        .iov_len = sizeof(message),
    };
    unsigned char control[CMSG_SPACE(sizeof(int))];
    memset(control, 0, sizeof(control));

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == NULL)
        return false;

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &passed_fd, sizeof(passed_fd));

    ssize_t sent;
    do {
        sent = sendmsg(socket_fd, &msg, MSG_NOSIGNAL);
    } while (sent < 0 && errno == EINTR);

    return sent == (ssize_t) sizeof(message);
}

static bool
build_socket_paths(char *runtime_directory,
                   size_t runtime_directory_size,
                   char *socket_path,
                   size_t socket_path_size)
{
    const char *xdg_runtime = getenv("XDG_RUNTIME_DIR");
    char fallback[64];

    if (xdg_runtime == NULL || *xdg_runtime == '\0') {
        int written = snprintf(fallback, sizeof(fallback), "/run/user/%lu",
                               (unsigned long) getuid());
        if (written < 0 || (size_t) written >= sizeof(fallback))
            return false;
        xdg_runtime = fallback;
    }

    int dir_written = snprintf(runtime_directory,
                               runtime_directory_size,
                               "%s/%s",
                               xdg_runtime,
                               GOREE_TERMINAL_HOST_RUNTIME_DIR);
    if (dir_written < 0 || (size_t) dir_written >= runtime_directory_size)
        return false;

    int socket_written = snprintf(socket_path,
                                  socket_path_size,
                                  "%s/%s",
                                  runtime_directory,
                                  GOREE_TERMINAL_HOST_SOCKET_NAME);
    if (socket_written < 0 || (size_t) socket_written >= socket_path_size)
        return false;

    return true;
}

static bool
ensure_private_runtime_directory(const char *path)
{
    struct stat st;

    if (lstat(path, &st) == 0) {
        if (!S_ISDIR(st.st_mode) || st.st_uid != getuid()) {
            errno = EPERM;
            return false;
        }
        if (chmod(path, S_IRWXU) != 0)
            return false;
        return true;
    }

    if (errno != ENOENT)
        return false;

    return mkdir(path, S_IRWXU) == 0;
}

static int
connect_to_unix_socket(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return -1;

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    if (strlen(path) >= sizeof(address.sun_path)) {
        close(fd);
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(address.sun_path, path, strlen(path) + 1);
    if (connect(fd, (struct sockaddr *) &address, sizeof(address)) != 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    return fd;
}

static bool
prepare_socket_path(const char *path)
{
    struct stat st;

    if (lstat(path, &st) != 0)
        return errno == ENOENT;

    if (!S_ISSOCK(st.st_mode) || st.st_uid != getuid()) {
        errno = EPERM;
        return false;
    }

    int probe = connect_to_unix_socket(path);
    if (probe >= 0) {
        close(probe);
        errno = EADDRINUSE;
        return false;
    }

    if (errno != ECONNREFUSED && errno != ENOENT)
        return false;

    return unlink(path) == 0 || errno == ENOENT;
}

static bool
resolve_local_account(struct passwd *account,
                      char *buffer,
                      size_t buffer_size)
{
    struct passwd *result = NULL;
    int rc = getpwuid_r(getuid(), account, buffer, buffer_size, &result);
    if (rc != 0) {
        errno = rc;
        return false;
    }
    if (result == NULL) {
        errno = ENOENT;
        return false;
    }
    return true;
}

static bool
field_is_terminated(const char *field, size_t field_size)
{
    return memchr(field, '\0', field_size) != NULL;
}

static bool
valid_environment_name(const char *name)
{
    if (name == NULL || *name == '\0' ||
        !(('A' <= *name && *name <= 'Z') ||
          ('a' <= *name && *name <= 'z') || *name == '_'))
        return false;

    for (const char *cursor = name + 1; *cursor != '\0'; cursor++) {
        if (!(('A' <= *cursor && *cursor <= 'Z') ||
              ('a' <= *cursor && *cursor <= 'z') ||
              ('0' <= *cursor && *cursor <= '9') || *cursor == '_'))
            return false;
    }
    return true;
}

static bool
shell_is_approved(const char *shell, const struct passwd *account)
{
    if (shell == NULL || *shell == '\0' || shell[0] != '/')
        return false;

    struct stat st;
    if (stat(shell, &st) != 0 || !S_ISREG(st.st_mode) || access(shell, X_OK) != 0)
        return false;

    if (account->pw_shell != NULL && strcmp(shell, account->pw_shell) == 0)
        return true;

    bool approved = false;
    setusershell();
    const char *candidate;
    while ((candidate = getusershell()) != NULL) {
        if (strcmp(shell, candidate) == 0) {
            approved = true;
            break;
        }
    }
    endusershell();
    return approved;
}

static const char *
resolve_shell(const GoreeTerminalHostSpawnRequest *request,
              const struct passwd *account)
{
    if (request->shell_path[0] != '\0')
        return shell_is_approved(request->shell_path, account)
            ? request->shell_path
            : NULL;

    if (shell_is_approved(account->pw_shell, account))
        return account->pw_shell;
    if (access("/bin/sh", X_OK) == 0)
        return "/bin/sh";
    return NULL;
}

static const char *
resolve_working_directory(const GoreeTerminalHostSpawnRequest *request,
                          const struct passwd *account)
{
    const char *directory = request->working_directory[0] != '\0'
        ? request->working_directory
        : account->pw_dir;
    struct stat st;

    if (directory == NULL || directory[0] != '/' ||
        stat(directory, &st) != 0 || !S_ISDIR(st.st_mode) ||
        access(directory, X_OK) != 0)
        return NULL;

    return directory;
}

static bool
validate_spawn_request(const GoreeTerminalHostSpawnRequest *request)
{
    if (request->header.magic != GOREE_TERMINAL_HOST_PROTOCOL_MAGIC ||
        request->header.version != GOREE_TERMINAL_HOST_PROTOCOL_VERSION ||
        request->header.type != GOREE_TERMINAL_HOST_MESSAGE_SPAWN_REQUEST ||
        request->header.value != 0)
        return false;

    if (request->header.rows > USHRT_MAX || request->header.columns > USHRT_MAX)
        return false;

    if (request->environment_policy != GOREE_TERMINAL_HOST_ENVIRONMENT_INHERIT_SAFE &&
        request->environment_policy != GOREE_TERMINAL_HOST_ENVIRONMENT_CLEAN)
        return false;
    if (request->environment_count > GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX)
        return false;

    if (!field_is_terminated(request->shell_path, sizeof(request->shell_path)) ||
        !field_is_terminated(request->working_directory,
                             sizeof(request->working_directory)))
        return false;

    if (request->shell_path[0] != '\0' && request->shell_path[0] != '/')
        return false;
    if (request->working_directory[0] != '\0' &&
        request->working_directory[0] != '/')
        return false;

    for (uint32_t index = 0; index < request->environment_count; index++) {
        if (!field_is_terminated(request->environment_names[index],
                                 sizeof(request->environment_names[index])) ||
            !valid_environment_name(request->environment_names[index]))
            return false;

        for (uint32_t previous = 0; previous < index; previous++) {
            if (strcmp(request->environment_names[index],
                       request->environment_names[previous]) == 0)
                return false;
        }
    }

    for (uint32_t index = request->environment_count;
         index < GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX;
         index++) {
        if (request->environment_names[index][0] != '\0')
            return false;
    }

    return true;
}

static char *
capture_environment_value(const char *name)
{
    const char *value = getenv(name);
    return value != NULL ? strdup(value) : NULL;
}

static bool
set_environment_value(const char *name, const char *value)
{
    return value == NULL || setenv(name, value, 1) == 0;
}

static bool
rebuild_environment(const GoreeTerminalHostSpawnRequest *request,
                    const struct passwd *account,
                    const char *shell,
                    const char *working_directory)
{
    size_t safe_count = 0;
    while (safe_environment_names[safe_count] != NULL)
        safe_count++;

    char *safe_values[6] = {0};
    char *allowed_values[GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX] = {0};

    for (size_t index = 0; index < safe_count; index++)
        safe_values[index] = capture_environment_value(safe_environment_names[index]);
    for (uint32_t index = 0; index < request->environment_count; index++)
        allowed_values[index] = capture_environment_value(request->environment_names[index]);

    if (clearenv() != 0)
        goto failure;

    const char *path_value = safe_values[0] != NULL
        ? safe_values[0]
        : "/usr/local/bin:/usr/bin:/bin";

    if (setenv("HOME", account->pw_dir != NULL ? account->pw_dir : "/", 1) != 0 ||
        setenv("USER", account->pw_name != NULL ? account->pw_name : "", 1) != 0 ||
        setenv("LOGNAME", account->pw_name != NULL ? account->pw_name : "", 1) != 0 ||
        setenv("SHELL", shell, 1) != 0 ||
        setenv("PWD", working_directory, 1) != 0 ||
        setenv("TERM", "xterm-256color", 1) != 0 ||
        setenv("COLORTERM", "truecolor", 1) != 0 ||
        setenv("PATH", path_value, 1) != 0)
        goto failure;

    if (request->environment_policy == GOREE_TERMINAL_HOST_ENVIRONMENT_INHERIT_SAFE) {
        for (size_t index = 1; index < safe_count; index++) {
            if (!set_environment_value(safe_environment_names[index], safe_values[index]))
                goto failure;
        }
    }

    for (uint32_t index = 0; index < request->environment_count; index++) {
        if (!set_environment_value(request->environment_names[index], allowed_values[index]))
            goto failure;
    }

    for (size_t index = 0; index < safe_count; index++)
        free(safe_values[index]);
    for (uint32_t index = 0; index < request->environment_count; index++)
        free(allowed_values[index]);
    return true;

failure:
    for (size_t index = 0; index < safe_count; index++)
        free(safe_values[index]);
    for (uint32_t index = 0; index < request->environment_count; index++)
        free(allowed_values[index]);
    return false;
}

static void
exec_shell(int slave_fd,
           int control_fd,
           const struct passwd *account,
           const GoreeTerminalHostSpawnRequest *request,
           const char *shell,
           const char *working_directory)
{
    close(control_fd);

    if (setsid() < 0)
        _exit(125);
    if (ioctl(slave_fd, TIOCSCTTY, 0) != 0)
        _exit(125);

    if (dup2(slave_fd, STDIN_FILENO) < 0 ||
        dup2(slave_fd, STDOUT_FILENO) < 0 ||
        dup2(slave_fd, STDERR_FILENO) < 0)
        _exit(125);
    if (slave_fd > STDERR_FILENO)
        close(slave_fd);

    if (tcsetpgrp(STDIN_FILENO, getpgrp()) != 0)
        _exit(125);
    if (chdir(working_directory) != 0)
        _exit(125);
    if (!rebuild_environment(request, account, shell, working_directory))
        _exit(125);

    const char *argv0 = strrchr(shell, '/');
    argv0 = argv0 != NULL ? argv0 + 1 : shell;
    execl(shell, argv0, (char *) NULL);
    _exit(127);
}

static void
handle_session(int client_fd)
{
    GoreeTerminalHostSpawnRequest request;
    ssize_t received = read_full(client_fd, &request, sizeof(request));
    if (received != (ssize_t) sizeof(request)) {
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR,
                            received < 0 ? errno : EPROTO);
        return;
    }

    if (!validate_spawn_request(&request)) {
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, EPROTO);
        return;
    }

    struct passwd account;
    char account_buffer[16384];
    if (!resolve_local_account(&account, account_buffer, sizeof(account_buffer))) {
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, errno);
        return;
    }

    const char *shell = resolve_shell(&request, &account);
    if (shell == NULL) {
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, EPERM);
        return;
    }

    const char *working_directory = resolve_working_directory(&request, &account);
    if (working_directory == NULL) {
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, ENOENT);
        return;
    }

    struct winsize window_size = {
        .ws_row = (unsigned short) (request.header.rows != 0 ? request.header.rows : 24),
        .ws_col = (unsigned short) (request.header.columns != 0 ? request.header.columns : 80),
        .ws_xpixel = 0,
        .ws_ypixel = 0,
    };

    int master_fd = -1;
    int slave_fd = -1;
    if (openpty(&master_fd, &slave_fd, NULL, NULL, &window_size) != 0) {
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, errno);
        return;
    }

    pid_t child = fork();
    if (child < 0) {
        int saved_errno = errno;
        close(master_fd);
        close(slave_fd);
        (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, saved_errno);
        return;
    }

    if (child == 0) {
        close(master_fd);
        exec_shell(slave_fd,
                   client_fd,
                   &account,
                   &request,
                   shell,
                   working_directory);
    }

    close(slave_fd);

    if (!send_message_with_fd(client_fd,
                              GOREE_TERMINAL_HOST_MESSAGE_SPAWN_RESPONSE,
                              (int32_t) child,
                              master_fd)) {
        close(master_fd);
        (void) kill(child, SIGHUP);
        (void) waitpid(child, NULL, 0);
        return;
    }
    close(master_fd);

    int status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno == EINTR)
            continue;
        status = 125 << 8;
        break;
    }

    (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_EXIT, status);
}

static bool
client_is_same_user(int client_fd)
{
#ifdef SO_PEERCRED
    struct ucred credentials;
    socklen_t length = sizeof(credentials);
    memset(&credentials, 0, sizeof(credentials));

    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &credentials, &length) != 0)
        return false;
    return credentials.uid == getuid();
#else
    (void) client_fd;
    errno = ENOTSUP;
    return false;
#endif
}

static void
cleanup_socket(void)
{
    if (agent_listen_fd >= 0) {
        close(agent_listen_fd);
        agent_listen_fd = -1;
    }
    if (agent_socket_path[0] != '\0')
        (void) unlink(agent_socket_path);
}

static void
termination_signal(int signal_number)
{
    cleanup_socket();
    _exit(128 + signal_number);
}

static bool
install_signal_handlers(void)
{
    struct sigaction terminate_action;
    memset(&terminate_action, 0, sizeof(terminate_action));
    terminate_action.sa_handler = termination_signal;
    sigemptyset(&terminate_action.sa_mask);

    if (sigaction(SIGINT, &terminate_action, NULL) != 0 ||
        sigaction(SIGTERM, &terminate_action, NULL) != 0)
        return false;

    /* SIGCHLD deliberately retains its default disposition. handle_session()
     * synchronously waitpid(2)s the exact child so it can send authoritative
     * EXIT evidence over the control channel. Ignoring SIGCHLD on Linux may
     * auto-reap the child and races that evidence path. */
    struct sigaction ignore_pipe;
    memset(&ignore_pipe, 0, sizeof(ignore_pipe));
    ignore_pipe.sa_handler = SIG_IGN;
    sigemptyset(&ignore_pipe.sa_mask);

    if (sigaction(SIGPIPE, &ignore_pipe, NULL) != 0)
        return false;

    return true;
}

static int
create_listening_socket(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
        return -1;

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    if (strlen(path) >= sizeof(address.sun_path)) {
        close(fd);
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(address.sun_path, path, strlen(path) + 1);

    if (bind(fd, (struct sockaddr *) &address, sizeof(address)) != 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }

    if (chmod(path, S_IRUSR | S_IWUSR) != 0 || listen(fd, 16) != 0) {
        int saved_errno = errno;
        close(fd);
        (void) unlink(path);
        errno = saved_errno;
        return -1;
    }

    return fd;
}

static void
print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [--print-socket] [--help]\n"
            "Runs the GoreeCloud Terminal local host-session agent.\n",
            program);
}

int
main(int argc, char **argv)
{
    char runtime_directory[PATH_MAX];
    char socket_path[PATH_MAX];

    if (!build_socket_paths(runtime_directory,
                            sizeof(runtime_directory),
                            socket_path,
                            sizeof(socket_path))) {
        fprintf(stderr, "Unable to construct host-agent socket path.\n");
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--print-socket") == 0) {
        puts(socket_path);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc != 1) {
        print_usage(argv[0]);
        return 2;
    }

    if (!ensure_private_runtime_directory(runtime_directory)) {
        perror("Unable to prepare private runtime directory");
        return 1;
    }
    if (!prepare_socket_path(socket_path)) {
        perror("Unable to prepare host-agent socket");
        return 1;
    }

    if (strlen(socket_path) >= sizeof(agent_socket_path)) {
        fprintf(stderr, "Host-agent socket path is too long.\n");
        return 1;
    }
    memcpy(agent_socket_path, socket_path, strlen(socket_path) + 1);

    if (atexit(cleanup_socket) != 0) {
        fprintf(stderr, "Unable to register host-agent cleanup.\n");
        return 1;
    }
    if (!install_signal_handlers()) {
        perror("Unable to install signal handlers");
        return 1;
    }

    agent_listen_fd = create_listening_socket(socket_path);
    if (agent_listen_fd < 0) {
        perror("Unable to create host-agent socket");
        return 1;
    }

    for (;;) {
        int client_fd = accept4(agent_listen_fd, NULL, NULL, SOCK_CLOEXEC);
        if (client_fd < 0) {
            if (errno == EINTR)
                continue;
            perror("Host-agent accept failed");
            return 1;
        }

        if (!client_is_same_user(client_fd)) {
            (void) send_message(client_fd, GOREE_TERMINAL_HOST_MESSAGE_ERROR, EPERM);
            close(client_fd);
            continue;
        }

        handle_session(client_fd);
        close(client_fd);
    }
}
