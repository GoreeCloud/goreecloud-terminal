#pragma once

#include <glib.h>

#include "host-session-client.h"
#include "session-profile.h"

typedef struct {
    const GoreeTerminalSessionProfile *profile;
    GoreeTerminalHostLaunchContext host_context;
    guint scrollback_lines;
    const char *theme_id;
} GoreeTerminalProfileRuntime;

const GoreeTerminalSessionProfile *goree_terminal_profile_find(
    const GPtrArray *profiles,
    const char *profile_id);

gboolean goree_terminal_profile_runtime_prepare(
    const GoreeTerminalSessionProfile *profile,
    GoreeTerminalProfileRuntime *runtime,
    GError **error);
