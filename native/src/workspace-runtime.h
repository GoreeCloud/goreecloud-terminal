#pragma once

#include <glib.h>

#include "profile-runtime.h"
#include "workspace-store.h"

typedef struct {
    guint tab_index;
    guint pane_index;
    gboolean initially_active;
    const char *tab_title;
    GoreeTerminalSplitOrientation orientation;
    GoreeTerminalProfileRuntime profile_runtime;
} GoreeTerminalWorkspacePaneRuntime;

typedef struct {
    const GoreeTerminalWorkspace *workspace;
    GPtrArray *panes;
    guint tab_count;
} GoreeTerminalWorkspaceRuntime;

gboolean goree_terminal_workspace_runtime_prepare(
    const GoreeTerminalWorkspace *workspace,
    const GPtrArray *profiles,
    GoreeTerminalWorkspaceRuntime *runtime,
    GError **error);

void goree_terminal_workspace_runtime_clear(
    GoreeTerminalWorkspaceRuntime *runtime);
