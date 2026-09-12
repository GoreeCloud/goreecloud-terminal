#ifndef GOREECLOUD_TERMINAL_HOST_SESSION_PROTOCOL_H
#define GOREECLOUD_TERMINAL_HOST_SESSION_PROTOCOL_H

#include <stdint.h>

#define GOREE_TERMINAL_HOST_PROTOCOL_MAGIC UINT32_C(0x47435448) /* GCTH */
#define GOREE_TERMINAL_HOST_PROTOCOL_VERSION UINT16_C(2)
#define GOREE_TERMINAL_HOST_RUNTIME_DIR "goreecloud-terminal"
#define GOREE_TERMINAL_HOST_SOCKET_NAME "host-agent.sock"

#define GOREE_TERMINAL_HOST_SHELL_PATH_MAX 256
#define GOREE_TERMINAL_HOST_WORKING_DIRECTORY_MAX 1024
#define GOREE_TERMINAL_HOST_ENVIRONMENT_NAME_MAX 64
#define GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX 16

typedef enum {
    GOREE_TERMINAL_HOST_MESSAGE_SPAWN_REQUEST = 1,
    GOREE_TERMINAL_HOST_MESSAGE_SPAWN_RESPONSE = 2,
    GOREE_TERMINAL_HOST_MESSAGE_EXIT = 3,
    GOREE_TERMINAL_HOST_MESSAGE_ERROR = 4,
} GoreeTerminalHostMessageType;

typedef enum {
    GOREE_TERMINAL_HOST_ENVIRONMENT_INHERIT_SAFE = 0,
    GOREE_TERMINAL_HOST_ENVIRONMENT_CLEAN = 1,
} GoreeTerminalHostEnvironmentPolicy;

/*
 * Local-only, same-machine protocol. All numeric fields use native byte order
 * because the client and host agent execute on the same kernel/architecture.
 *
 * The protocol is intentionally not a command RPC. A SPAWN_REQUEST may carry
 * only bounded launch metadata for an interactive shell session:
 *
 * - a requested login-shell path, which the host agent independently validates
 *   against the local account and approved login shells;
 * - an absolute initial working directory, independently validated by the host;
 * - an environment policy; and
 * - environment variable names whose values, if allowed, are resolved by the
 *   host agent from its own environment.
 *
 * It never carries command strings, terminal contents, shell history,
 * environment values, passwords, credentials, private keys, tokens, SSH
 * secrets, or other reusable authentication material.
 *
 * GoreeTerminalHostMessage remains the fixed control/event header. For
 * SPAWN_RESPONSE, value is the host child PID and one PTY-master descriptor is
 * attached using SCM_RIGHTS. For EXIT, value is the raw waitpid(2) status. For
 * ERROR, value is a positive errno-style code.
 */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    int32_t value;
    uint32_t rows;
    uint32_t columns;
} GoreeTerminalHostMessage;

/*
 * Version 2 SPAWN_REQUEST payload. Empty shell_path selects the authenticated
 * account's login shell. Empty working_directory selects the account home.
 * environment_names contains names only; unused entries must be NUL-filled.
 */
typedef struct {
    GoreeTerminalHostMessage header;
    uint32_t environment_policy;
    uint32_t environment_count;
    char shell_path[GOREE_TERMINAL_HOST_SHELL_PATH_MAX];
    char working_directory[GOREE_TERMINAL_HOST_WORKING_DIRECTORY_MAX];
    char environment_names[GOREE_TERMINAL_HOST_ENVIRONMENT_COUNT_MAX]
                          [GOREE_TERMINAL_HOST_ENVIRONMENT_NAME_MAX];
} GoreeTerminalHostSpawnRequest;

_Static_assert(sizeof(GoreeTerminalHostMessage) == 20,
               "host-session protocol header size changed");
_Static_assert(sizeof(GoreeTerminalHostSpawnRequest) == 2332,
               "host-session spawn request size changed");

#endif
