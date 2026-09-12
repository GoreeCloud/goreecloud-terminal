#include "workspace-view.h"

#define WORKSPACE_VIEW_STATE_KEY "goreecloud-workspace-view-state"
#define WORKSPACE_PANE_MIN_PIXELS 80

typedef struct _WorkspaceViewState WorkspaceViewState;

typedef struct {
    WorkspaceViewState *state;
    GtkWidget *frame;
    GtkWidget *content;
    guint tab_index;
    guint pane_index;
} WorkspacePaneBinding;

struct _WorkspaceViewState {
    GtkWidget *notebook;
    GPtrArray *bindings;
    guint active_index;
};

static void
workspace_pane_binding_free(gpointer data)
{
    g_free(data);
}

static void
workspace_view_state_free(gpointer data)
{
    WorkspaceViewState *state = data;
    if (state == NULL)
        return;
    g_clear_pointer(&state->bindings, g_ptr_array_unref);
    g_free(state);
}

static WorkspaceViewState *
workspace_view_state(GtkWidget *workspace_view)
{
    if (!GTK_IS_NOTEBOOK(workspace_view))
        return NULL;
    return g_object_get_data(G_OBJECT(workspace_view), WORKSPACE_VIEW_STATE_KEY);
}

static void
set_active_binding(WorkspaceViewState *state, guint index, gboolean focus_content)
{
    if (state == NULL || state->bindings == NULL || index >= state->bindings->len)
        return;

    for (guint i = 0; i < state->bindings->len; i++) {
        WorkspacePaneBinding *binding = g_ptr_array_index(state->bindings, i);
        if (i == index)
            gtk_widget_add_css_class(binding->frame, "glaze-active-pane");
        else
            gtk_widget_remove_css_class(binding->frame, "glaze-active-pane");
    }

    state->active_index = index;
    WorkspacePaneBinding *active = g_ptr_array_index(state->bindings, index);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), (int) active->tab_index);

    if (focus_content)
        gtk_widget_grab_focus(active->content);
}

static void
pane_focus_enter(GtkEventControllerFocus *controller, gpointer user_data)
{
    WorkspacePaneBinding *binding = user_data;
    WorkspaceViewState *state = binding != NULL ? binding->state : NULL;

    (void) controller;
    if (state == NULL || state->bindings == NULL)
        return;

    for (guint i = 0; i < state->bindings->len; i++) {
        if (g_ptr_array_index(state->bindings, i) == binding) {
            set_active_binding(state, i, FALSE);
            return;
        }
    }
}

static GtkWidget *
create_pane_frame(
    WorkspaceViewState *state,
    const GoreeTerminalWorkspacePaneRuntime *pane_runtime,
    GoreeTerminalWorkspacePaneFactory pane_factory,
    gpointer user_data,
    GError **error)
{
    GtkWidget *content = pane_factory(pane_runtime, user_data, error);
    if (content == NULL)
        return NULL;

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "glaze-workspace-pane");
    gtk_widget_set_hexpand(frame, TRUE);
    gtk_widget_set_vexpand(frame, TRUE);
    gtk_widget_set_hexpand(content, TRUE);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_frame_set_child(GTK_FRAME(frame), content);

    WorkspacePaneBinding *binding = g_new0(WorkspacePaneBinding, 1);
    binding->state = state;
    binding->frame = frame;
    binding->content = content;
    binding->tab_index = pane_runtime->tab_index;
    binding->pane_index = pane_runtime->pane_index;
    g_ptr_array_add(state->bindings, binding);

    GtkEventController *focus = gtk_event_controller_focus_new();
    g_signal_connect(focus, "enter", G_CALLBACK(pane_focus_enter), binding);
    gtk_widget_add_controller(content, focus);

    char *accessible = g_strdup_printf(
        "Terminal workspace pane %u in tab %u",
        pane_runtime->pane_index + 1,
        pane_runtime->tab_index + 1);
    gtk_accessible_update_property(
        GTK_ACCESSIBLE(frame),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        accessible,
        -1);
    g_free(accessible);
    return frame;
}

static GtkWidget *
build_tab_layout(
    WorkspaceViewState *state,
    const GoreeTerminalWorkspaceRuntime *runtime,
    guint tab_index,
    GoreeTerminalWorkspacePaneFactory pane_factory,
    gpointer user_data,
    GError **error)
{
    GtkWidget *layout = NULL;
    GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL;
    guint pane_count = 0;

    for (guint i = 0; i < runtime->panes->len; i++) {
        const GoreeTerminalWorkspacePaneRuntime *pane_runtime =
            g_ptr_array_index(runtime->panes, i);
        if (pane_runtime->tab_index != tab_index)
            continue;

        orientation = pane_runtime->orientation == GOREE_TERMINAL_SPLIT_VERTICAL
            ? GTK_ORIENTATION_VERTICAL
            : GTK_ORIENTATION_HORIZONTAL;

        GtkWidget *frame = create_pane_frame(
            state,
            pane_runtime,
            pane_factory,
            user_data,
            error);
        if (frame == NULL)
            return NULL;

        if (layout == NULL) {
            layout = frame;
        } else {
            GtkWidget *paned = gtk_paned_new(orientation);
            gtk_widget_add_css_class(paned, "glaze-workspace-split");
            gtk_widget_set_hexpand(paned, TRUE);
            gtk_widget_set_vexpand(paned, TRUE);
            gtk_paned_set_start_child(GTK_PANED(paned), layout);
            gtk_paned_set_end_child(GTK_PANED(paned), frame);
            gtk_paned_set_resize_start_child(GTK_PANED(paned), TRUE);
            gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
            gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
            gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);
            layout = paned;
        }
        pane_count++;
    }

    if (pane_count == 0) {
        g_set_error(
            error,
            G_OPTION_ERROR,
            G_OPTION_ERROR_BAD_VALUE,
            "Workspace tab %u contains no runtime panes.",
            tab_index + 1);
        return NULL;
    }
    return layout;
}

GtkWidget *
goree_terminal_workspace_view_new(
    const GoreeTerminalWorkspaceRuntime *runtime,
    GoreeTerminalWorkspacePaneFactory pane_factory,
    gpointer user_data,
    GError **error)
{
    g_return_val_if_fail(runtime != NULL, NULL);
    g_return_val_if_fail(runtime->panes != NULL, NULL);
    g_return_val_if_fail(pane_factory != NULL, NULL);

    WorkspaceViewState *state = g_new0(WorkspaceViewState, 1);
    state->bindings = g_ptr_array_new_with_free_func(workspace_pane_binding_free);
    state->notebook = gtk_notebook_new();
    gtk_widget_add_css_class(state->notebook, "glaze-workspace-tabs");
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(state->notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(state->notebook), FALSE);

    for (guint tab_index = 0; tab_index < runtime->tab_count; tab_index++) {
        GtkWidget *layout = build_tab_layout(
            state,
            runtime,
            tab_index,
            pane_factory,
            user_data,
            error);
        if (layout == NULL) {
            g_object_unref(state->notebook);
            workspace_view_state_free(state);
            return NULL;
        }

        const char *title = NULL;
        for (guint i = 0; i < runtime->panes->len; i++) {
            const GoreeTerminalWorkspacePaneRuntime *pane_runtime =
                g_ptr_array_index(runtime->panes, i);
            if (pane_runtime->tab_index == tab_index) {
                title = pane_runtime->tab_title;
                break;
            }
        }
        if (title == NULL || *title == '\0')
            title = "Workspace";

        GtkWidget *label = gtk_label_new(title);
        gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), layout, label);
    }

    if (state->bindings->len == 0) {
        g_set_error_literal(
            error,
            G_OPTION_ERROR,
            G_OPTION_ERROR_BAD_VALUE,
            "Workspace runtime contains no panes.");
        g_object_unref(state->notebook);
        workspace_view_state_free(state);
        return NULL;
    }

    guint initial = 0;
    for (guint i = 0; i < runtime->panes->len && i < state->bindings->len; i++) {
        const GoreeTerminalWorkspacePaneRuntime *pane_runtime =
            g_ptr_array_index(runtime->panes, i);
        if (pane_runtime->initially_active) {
            initial = i;
            break;
        }
    }
    set_active_binding(state, initial, FALSE);

    g_object_set_data_full(
        G_OBJECT(state->notebook),
        WORKSPACE_VIEW_STATE_KEY,
        state,
        workspace_view_state_free);
    return state->notebook;
}

GtkWidget *
goree_terminal_workspace_view_get_active_pane(GtkWidget *workspace_view)
{
    WorkspaceViewState *state = workspace_view_state(workspace_view);
    if (state == NULL || state->bindings == NULL ||
        state->active_index >= state->bindings->len)
        return NULL;

    WorkspacePaneBinding *binding = g_ptr_array_index(
        state->bindings,
        state->active_index);
    return binding->content;
}

static gboolean
focus_relative(GtkWidget *workspace_view, int direction)
{
    WorkspaceViewState *state = workspace_view_state(workspace_view);
    if (state == NULL || state->bindings == NULL || state->bindings->len == 0)
        return FALSE;

    gint length = (gint) state->bindings->len;
    gint next = ((gint) state->active_index + direction) % length;
    if (next < 0)
        next += length;
    set_active_binding(state, (guint) next, TRUE);
    return TRUE;
}

gboolean
goree_terminal_workspace_view_focus_next(GtkWidget *workspace_view)
{
    return focus_relative(workspace_view, 1);
}

gboolean
goree_terminal_workspace_view_focus_previous(GtkWidget *workspace_view)
{
    return focus_relative(workspace_view, -1);
}

static GtkWidget *
nearest_paned_ancestor(GtkWidget *widget, GtkWidget **direct_branch)
{
    GtkWidget *branch = widget;
    GtkWidget *parent = gtk_widget_get_parent(branch);

    while (parent != NULL) {
        if (GTK_IS_PANED(parent)) {
            if (direct_branch != NULL)
                *direct_branch = branch;
            return parent;
        }
        branch = parent;
        parent = gtk_widget_get_parent(branch);
    }
    return NULL;
}

gboolean
goree_terminal_workspace_view_resize_active(
    GtkWidget *workspace_view,
    int delta_pixels)
{
    WorkspaceViewState *state = workspace_view_state(workspace_view);
    if (state == NULL || state->bindings == NULL ||
        state->active_index >= state->bindings->len || delta_pixels == 0)
        return FALSE;

    WorkspacePaneBinding *binding = g_ptr_array_index(
        state->bindings,
        state->active_index);
    GtkWidget *branch = NULL;
    GtkWidget *paned_widget = nearest_paned_ancestor(binding->frame, &branch);
    if (!GTK_IS_PANED(paned_widget))
        return FALSE;

    GtkPaned *paned = GTK_PANED(paned_widget);
    GtkWidget *start = gtk_paned_get_start_child(paned);
    int signed_delta = branch == start ? delta_pixels : -delta_pixels;
    int position = gtk_paned_get_position(paned);
    GtkOrientation orientation = gtk_orientable_get_orientation(GTK_ORIENTABLE(paned));
    int total = orientation == GTK_ORIENTATION_HORIZONTAL
        ? gtk_widget_get_width(paned_widget)
        : gtk_widget_get_height(paned_widget);

    if (total <= WORKSPACE_PANE_MIN_PIXELS * 2)
        return FALSE;

    int next = CLAMP(
        position + signed_delta,
        WORKSPACE_PANE_MIN_PIXELS,
        total - WORKSPACE_PANE_MIN_PIXELS);
    if (next == position)
        return FALSE;

    gtk_paned_set_position(paned, next);
    return TRUE;
}
