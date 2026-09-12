#include "remote-target-runtime.h"

#include <string.h>

GoreeTerminalRemoteTargetStatus *
goree_terminal_remote_target_status_new(
    const GoreeTerminalRemoteTargetRecord *record,
    const GoreeTerminalRemotePrivacyAdapter *privacy_adapter,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix,
    GError **error)
{
    g_return_val_if_fail(record != NULL, NULL);

    if (!goree_terminal_remote_target_record_validate(record, error))
        return NULL;

    GoreeTerminalRemoteTargetStatus *status = g_new0(
        GoreeTerminalRemoteTargetStatus,
        1);
    status->record = goree_terminal_remote_target_record_copy(record);

    if (!goree_terminal_remote_session_init_from_privacy_adapter(
            &status->session,
            &status->record->target,
            privacy_adapter,
            security,
            now_unix,
            error)) {
        goree_terminal_remote_target_status_free(status);
        return NULL;
    }

    return status;
}

void
goree_terminal_remote_target_status_free(
    GoreeTerminalRemoteTargetStatus *status)
{
    if (status == NULL)
        return;

    goree_terminal_remote_target_record_free(status->record);
    memset(&status->session, 0, sizeof(status->session));
    g_free(status);
}

GPtrArray *
goree_terminal_remote_target_catalog_load(
    const GoreeTerminalRemotePrivacyAdapter *privacy_adapter,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix,
    GError **error)
{
    GPtrArray *records = goree_terminal_remote_targets_load(error);
    if (records == NULL)
        return NULL;

    GPtrArray *catalog = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_remote_target_status_free);

    for (guint index = 0; index < records->len; index++) {
        const GoreeTerminalRemoteTargetRecord *record = g_ptr_array_index(
            records,
            index);
        GoreeTerminalRemoteTargetStatus *status =
            goree_terminal_remote_target_status_new(
                record,
                privacy_adapter,
                security,
                now_unix,
                error);
        if (status == NULL) {
            g_ptr_array_unref(records);
            g_ptr_array_unref(catalog);
            return NULL;
        }
        g_ptr_array_add(catalog, status);
    }

    g_ptr_array_unref(records);
    return catalog;
}

const GoreeTerminalRemoteTargetStatus *
goree_terminal_remote_target_catalog_find(
    const GPtrArray *catalog,
    const char *id)
{
    if (catalog == NULL || id == NULL || *id == '\0')
        return NULL;

    for (guint index = 0; index < catalog->len; index++) {
        const GoreeTerminalRemoteTargetStatus *status = g_ptr_array_index(
            (GPtrArray *) catalog,
            index);
        if (status != NULL && status->record != NULL &&
            g_strcmp0(status->record->id, id) == 0)
            return status;
    }
    return NULL;
}

const char *
goree_terminal_remote_target_status_label(
    const GoreeTerminalRemoteTargetStatus *status)
{
    if (status == NULL)
        return "Remote target unavailable";

    switch (status->session.state) {
    case GOREE_TERMINAL_REMOTE_READY:
        return "Ready — authorized";
    case GOREE_TERMINAL_REMOTE_CONNECTING:
        return "Connecting";
    case GOREE_TERMINAL_REMOTE_HOST_KEY_VERIFICATION:
        return "Verifying host key";
    case GOREE_TERMINAL_REMOTE_AUTHENTICATING:
        return "Authenticating";
    case GOREE_TERMINAL_REMOTE_RUNNING:
        return "Connected";
    case GOREE_TERMINAL_REMOTE_DISCONNECTED:
        return "Disconnected";
    case GOREE_TERMINAL_REMOTE_RECONNECTING:
        return "Reconnecting — reauthorization required";
    case GOREE_TERMINAL_REMOTE_EXITED:
        return "Exited";
    case GOREE_TERMINAL_REMOTE_BLOCKED:
    default:
        break;
    }

    switch (status->session.privacy.decision) {
    case GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE:
        return "Blocked — Privacy Shield unavailable";
    case GOREE_TERMINAL_REMOTE_PRIVACY_DENY:
        return "Blocked — Privacy Shield denied access";
    case GOREE_TERMINAL_REMOTE_PRIVACY_REQUIRE_USER_DECISION:
        return "Blocked — Privacy Shield requires a user decision";
    case GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW_WITH_CONSTRAINTS:
        return "Blocked — Privacy Shield constraints are not satisfied";
    case GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW:
    default:
        return "Blocked — authorization evidence is not acceptable";
    }
}
