#include "remote-privacy-adapter.h"

#include <string.h>

static gboolean
default_unavailable_authorize(
    const GoreeTerminalRemoteTarget *target,
    gint64 now_unix,
    GoreeTerminalRemotePrivacyEvidence *evidence,
    gpointer user_data,
    GError **error)
{
    (void) now_unix;
    (void) user_data;

    memset(evidence, 0, sizeof(*evidence));
    evidence->decision = GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE;
    evidence->authority_verified = FALSE;
    evidence->runtime_accepted = FALSE;
    evidence->constraints_present = FALSE;
    evidence->constraints_satisfied = FALSE;
    evidence->expires_at_unix = 0;
    g_strlcpy(
        evidence->purpose,
        GOREE_TERMINAL_REMOTE_PURPOSE,
        sizeof(evidence->purpose));

    return goree_terminal_remote_target_binding(
        target,
        evidence->target_binding,
        error);
}

void
goree_terminal_remote_privacy_adapter_default(
    GoreeTerminalRemotePrivacyAdapter *adapter)
{
    g_return_if_fail(adapter != NULL);
    adapter->authorize = default_unavailable_authorize;
    adapter->user_data = NULL;
}

gboolean
goree_terminal_remote_privacy_adapter_query(
    const GoreeTerminalRemotePrivacyAdapter *adapter,
    const GoreeTerminalRemoteTarget *target,
    gint64 now_unix,
    GoreeTerminalRemotePrivacyEvidence *evidence,
    GError **error)
{
    g_return_val_if_fail(target != NULL, FALSE);
    g_return_val_if_fail(evidence != NULL, FALSE);

    GoreeTerminalRemotePrivacyAdapter fallback;
    if (adapter == NULL || adapter->authorize == NULL) {
        goree_terminal_remote_privacy_adapter_default(&fallback);
        adapter = &fallback;
    }

    memset(evidence, 0, sizeof(*evidence));
    if (!adapter->authorize(
            target,
            now_unix,
            evidence,
            adapter->user_data,
            error)) {
        memset(evidence, 0, sizeof(*evidence));
        return FALSE;
    }

    if (evidence->purpose[0] == '\0') {
        g_set_error_literal(
            error,
            G_OPTION_ERROR,
            G_OPTION_ERROR_BAD_VALUE,
            "Privacy Shield adapter returned evidence without a purpose binding.");
        memset(evidence, 0, sizeof(*evidence));
        return FALSE;
    }
    if (evidence->target_binding[0] == '\0') {
        g_set_error_literal(
            error,
            G_OPTION_ERROR,
            G_OPTION_ERROR_BAD_VALUE,
            "Privacy Shield adapter returned evidence without a destination binding.");
        memset(evidence, 0, sizeof(*evidence));
        return FALSE;
    }
    return TRUE;
}

gboolean
goree_terminal_remote_session_init_from_privacy_adapter(
    GoreeTerminalRemoteSession *session,
    const GoreeTerminalRemoteTarget *target,
    const GoreeTerminalRemotePrivacyAdapter *adapter,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix,
    GError **error)
{
    g_return_val_if_fail(session != NULL, FALSE);
    g_return_val_if_fail(target != NULL, FALSE);

    GoreeTerminalRemotePrivacyEvidence evidence;
    if (!goree_terminal_remote_privacy_adapter_query(
            adapter,
            target,
            now_unix,
            &evidence,
            error))
        return FALSE;

    goree_terminal_remote_session_init(
        session,
        target,
        &evidence,
        security,
        now_unix);
    return TRUE;
}
