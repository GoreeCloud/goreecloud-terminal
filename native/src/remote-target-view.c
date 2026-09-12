#include "remote-target-view.h"

static char *
destination_label(const GoreeTerminalRemoteTarget *target)
{
    if (target == NULL)
        return g_strdup("Destination unavailable");

    if (target->username[0] != '\0') {
        if (target->port == 22)
            return g_strdup_printf("%s@%s", target->username, target->host);
        return g_strdup_printf(
            "%s@%s:%u",
            target->username,
            target->host,
            target->port);
    }

    if (target->port == 22)
        return g_strdup(target->host);
    return g_strdup_printf("%s:%u", target->host, target->port);
}

static GtkWidget *
create_message_row(const char *title_text, const char *detail_text)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title = gtk_label_new(title_text);
    GtkWidget *detail = gtk_label_new(detail_text);

    gtk_widget_add_css_class(row, "glaze-workspace-pane");
    gtk_widget_set_margin_top(row, 12);
    gtk_widget_set_margin_bottom(row, 12);
    gtk_widget_set_margin_start(row, 12);
    gtk_widget_set_margin_end(row, 12);

    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(detail), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(detail), TRUE);
    gtk_widget_add_css_class(title, "heading");
    gtk_widget_add_css_class(detail, "dim-label");

    gtk_box_append(GTK_BOX(row), title);
    gtk_box_append(GTK_BOX(row), detail);
    return row;
}

static GtkWidget *
create_target_row(const GoreeTerminalRemoteTargetStatus *status)
{
    const GoreeTerminalRemoteTargetRecord *record = status->record;
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *name = gtk_label_new(record->name);
    GtkWidget *state = gtk_label_new(
        goree_terminal_remote_target_status_label(status));
    char *destination_text = destination_label(&record->target);
    GtkWidget *destination = gtk_label_new(destination_text);
    const char *coverage_text = goree_terminal_remote_coverage_label(
        &status->session.security);
    GtkWidget *coverage = gtk_label_new(coverage_text);
    char *accessible_text = g_strdup_printf(
        "%s, %s, %s, %s. Remote execution is not started from this status view.",
        record->name,
        destination_text,
        goree_terminal_remote_target_status_label(status),
        coverage_text);

    gtk_widget_add_css_class(row, "glaze-workspace-pane");
    if (status->session.state == GOREE_TERMINAL_REMOTE_BLOCKED)
        gtk_widget_add_css_class(row, "glaze-session-disconnected");
    else if (status->session.state == GOREE_TERMINAL_REMOTE_READY)
        gtk_widget_add_css_class(row, "glaze-session-host");

    gtk_widget_set_margin_top(row, 8);
    gtk_widget_set_margin_bottom(row, 8);
    gtk_widget_set_margin_start(row, 12);
    gtk_widget_set_margin_end(row, 12);

    gtk_widget_set_hexpand(name, TRUE);
    gtk_label_set_xalign(GTK_LABEL(name), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(state), 1.0f);
    gtk_label_set_xalign(GTK_LABEL(destination), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(coverage), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(destination), PANGO_ELLIPSIZE_MIDDLE);

    gtk_widget_add_css_class(name, "heading");
    gtk_widget_add_css_class(state, "dim-label");
    gtk_widget_add_css_class(destination, "dim-label");
    gtk_widget_add_css_class(coverage, "dim-label");

    gtk_box_append(GTK_BOX(header), name);
    gtk_box_append(GTK_BOX(header), state);
    gtk_box_append(GTK_BOX(row), header);
    gtk_box_append(GTK_BOX(row), destination);
    gtk_box_append(GTK_BOX(row), coverage);

    gtk_accessible_update_property(
        GTK_ACCESSIBLE(row),
        GTK_ACCESSIBLE_PROPERTY_LABEL,
        accessible_text,
        -1);
    gtk_widget_set_tooltip_text(row, accessible_text);

    g_free(accessible_text);
    g_free(destination_text);
    return row;
}

GtkWidget *
goree_terminal_remote_target_view_new(
    const GPtrArray *catalog,
    const char *load_error)
{
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    GtkWidget *heading = gtk_label_new("Saved Remote Targets");
    GtkWidget *explanation = gtk_label_new(
        "Remote targets contain destination metadata only. Authorization is evaluated separately by Privacy Shield, and this view never starts an SSH process.");
    GtkWidget *scroller = gtk_scrolled_window_new();
    GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

    gtk_widget_add_css_class(root, "glaze-workspace-tabs");
    gtk_widget_set_margin_top(root, 18);
    gtk_widget_set_margin_bottom(root, 18);
    gtk_widget_set_margin_start(root, 18);
    gtk_widget_set_margin_end(root, 18);

    gtk_label_set_xalign(GTK_LABEL(heading), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(explanation), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(explanation), TRUE);
    gtk_widget_add_css_class(heading, "title");
    gtk_widget_add_css_class(explanation, "dim-label");

    gtk_widget_set_hexpand(scroller, TRUE);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroller),
        GTK_POLICY_NEVER,
        GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), list);

    gtk_box_append(GTK_BOX(root), heading);
    gtk_box_append(GTK_BOX(root), explanation);
    gtk_box_append(GTK_BOX(root), scroller);

    if (load_error != NULL && *load_error != '\0') {
        gtk_box_append(
            GTK_BOX(list),
            create_message_row(
                "Remote targets are unavailable",
                "GoreeCloud Terminal refused to use the remote-target catalog because its private configuration could not be validated. No remote connection was attempted."));
        return root;
    }

    if (catalog == NULL || catalog->len == 0) {
        gtk_box_append(
            GTK_BOX(list),
            create_message_row(
                "No saved remote targets",
                "No remote destination is configured. GoreeCloud Terminal does not invent a default host."));
        return root;
    }

    for (guint index = 0; index < catalog->len; index++) {
        const GoreeTerminalRemoteTargetStatus *status = g_ptr_array_index(
            (GPtrArray *) catalog,
            index);
        if (status != NULL && status->record != NULL)
            gtk_box_append(GTK_BOX(list), create_target_row(status));
    }

    return root;
}
