#pragma once

#include <glib.h>

typedef enum {
    GOREE_TERMINAL_SPLIT_HORIZONTAL = 0,
    GOREE_TERMINAL_SPLIT_VERTICAL,
} GoreeTerminalSplitOrientation;

typedef struct {
    char *title;
    GoreeTerminalSplitOrientation orientation;
    GPtrArray *profile_ids;
} GoreeTerminalWorkspaceTab;

typedef struct {
    char *id;
    char *name;
    GPtrArray *tabs;
} GoreeTerminalWorkspace;

GoreeTerminalWorkspaceTab *goree_terminal_workspace_tab_new(
    const char *title,
    GoreeTerminalSplitOrientation orientation);
void goree_terminal_workspace_tab_free(GoreeTerminalWorkspaceTab *tab);

GoreeTerminalWorkspace *goree_terminal_workspace_new_default(void);
void goree_terminal_workspace_free(GoreeTerminalWorkspace *workspace);

gboolean goree_terminal_workspace_validate(
    const GoreeTerminalWorkspace *workspace,
    GError **error);

const char *goree_terminal_split_orientation_id(
    GoreeTerminalSplitOrientation orientation);

gboolean goree_terminal_split_orientation_from_id(
    const char *id,
    GoreeTerminalSplitOrientation *orientation);

GPtrArray *goree_terminal_workspaces_load(GError **error);

gboolean goree_terminal_workspaces_save(
    const GPtrArray *workspaces,
    GError **error);

const char *goree_terminal_workspaces_path(void);
