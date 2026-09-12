#ifndef GOREECLOUD_TERMINAL_HOST_SESSION_PROTOCOL_H
#define GOREECLOUD_TERMINAL_HOST_SESSION_PROTOCOL_H

#include <stdint.h>

#define GOREE_TERMINAL_HOST_PROTOCOL_MAGIC UINT32_C(0x47435448) /* GCTH */
#define GOREE_TERMINAL_HOST_PROTOCOL_VERSION UINT16_C(1)
#define GOREE_TERMINAL_HOST_RUNTIME_DIR "goreecloud-terminal"
#define GOREE_TERMINAL_HOST_SOCKET_NAME "host-agent.sock"

typedef enum {
    GOREE_TERMINAL_HOST_MESSAGE_SPAWN_REQUEST = 1,
    GOREE_TERMINAL_HOST_MESSAGE_SPAWN_RESPONSE = 2,
    GOREE_TERMINAL_HOST_MESSAGE_EXIT = 3,
    GOREE_TERMINAL_HOST_MESSAGE_ERROR = 4,
} GoreeTerminalHostMessageType;

/*
 * Local-only, same-machine protocol. All fields use native byte order because
 * the client and host agent execute on the same kernel/architecture. The
 * protocol intentionally carries no terminal contents, commands, credentials,
 * environment dump, working-directory history, or other session payloads.
 *
 * For SPAWN_REQUEST:
 *   value   = 0
 *   rows    = requested initial PTY rows (0 means agent default)
 *   columns = requested initial PTY columns (0 means agent default)
 *
 * For SPAWN_RESPONSE:
 *   value   = host child PID for diagnostic correlation only
 *   rows/columns = 0
 *   one PTY-master file descriptor is attached with SCM_RIGHTS
 *
 * For EXIT:
 *   value   = raw waitpid(2) status
 *
 * For ERROR:
 *   value   = positive errno-style error code
 */
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    int32_t value;
    uint32_t rows;
    uint32_t columns;
} GoreeTerminalHostMessage;

_Static_assert(sizeof(GoreeTerminalHostMessage) == 20,
               "host-session protocol header size changed");

#endif
