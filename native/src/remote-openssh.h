#pragma once

#include <glib.h>

#include "remote-session.h"

/*
 * Build argv for a standard interactive OpenSSH client. The result contains no
 * remote command, password, token, private-key bytes, or host-key bypass flag.
 * OpenSSH remains authoritative for ~/.ssh/config, ssh-agent interaction,
 * host-key verification, authentication, and connection semantics.
 */
gboolean goree_terminal_remote_openssh_build_argv(
    const GoreeTerminalRemoteTarget *target,
    char ***argv_out,
    GError **error);
