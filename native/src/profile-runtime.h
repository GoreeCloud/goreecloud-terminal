#pragma once

#include <glib.h>

#include "host-session-client.h"
#include "session-profile.h"
#include "workspace-store.h"

typedef struct {
    GPtrArray *profiles;
    GPtrArray *workspaces;
} GoreeTerminalRuntimeCatalog;

gboolean goree_terminal_runtime_catalog_load(
    GoreeTerminalRuntimeCatalog *catalog,
    GError **error);

void goree_terminal_runtime_catalog_clear(
    GoreeTerminalRuntimeCatalog *catalog);

const GoreeTerminalSessionProfile *goree_terminal_runtime_catalog_find_profile(
    const GoreeTerminalRuntimeCatalog *catalog,
    const char *profile_id);

const GoreeTerminalWorkspace *goree_terminal_runtime_catalog_find_workspace(
    const GoreeTerminalRuntimeCatalog *catalog,
    const char *workspace_id);

gboolean goree_terminal_runtime_catalog_validate(
    const GoreeTerminalRuntimeCatalog *catalog,
    GError **error);

void goree_terminal_runtime_launch_context_for_profile(
    const GoreeTerminalSessionProfile *profile,
    GoreeTerminalHostLaunchContext *context);
