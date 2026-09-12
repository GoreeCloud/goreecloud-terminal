#pragma once

#include <gtk/gtk.h>

#include "workspace-runtime.h"

typedef GtkWidget *(*GoreeTerminalWorkspacePaneFactory)(
    const GoreeTerminalWorkspacePaneRuntime *pane_runtime,
    gpointer user_data,
    GError **error);

/*
 * Build a GTK notebook for an already validated workspace runtime plan.
 * The returned widget owns its internal active-pane state. The pane factory is
 * invoked once for each planned pane and must not reinterpret profile policy.
 */
GtkWidget *goree_terminal_workspace_view_new(
    const GoreeTerminalWorkspaceRuntime *runtime,
    GoreeTerminalWorkspacePaneFactory pane_factory,
    gpointer user_data,
    GError **error);

GtkWidget *goree_terminal_workspace_view_get_active_pane(GtkWidget *workspace_view);

gboolean goree_terminal_workspace_view_focus_next(GtkWidget *workspace_view);
gboolean goree_terminal_workspace_view_focus_previous(GtkWidget *workspace_view);

/* Positive delta expands the active pane; negative delta contracts it. */
gboolean goree_terminal_workspace_view_resize_active(
    GtkWidget *workspace_view,
    int delta_pixels);
