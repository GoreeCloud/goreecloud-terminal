#pragma once

#include <glib.h>

#include "remote-session.h"

typedef gboolean (*GoreeTerminalRemotePrivacyAuthorizeFunc)(
    const GoreeTerminalRemoteTarget *target,
    gint64 now_unix,
    GoreeTerminalRemotePrivacyEvidence *evidence,
    gpointer user_data,
    GError **error);

typedef struct {
    GoreeTerminalRemotePrivacyAuthorizeFunc authorize;
    gpointer user_data;
} GoreeTerminalRemotePrivacyAdapter;

/*
 * The default adapter is intentionally non-authoritative. It binds the request
 * to the exact destination/purpose for explainability, but always returns a
 * Privacy Shield UNAVAILABLE decision with no accepted authority.
 */
void goree_terminal_remote_privacy_adapter_default(
    GoreeTerminalRemotePrivacyAdapter *adapter);

/*
 * Query an adapter for external Privacy Shield evidence. A TRUE return means
 * the adapter produced a syntactically usable evidence object; it does not mean
 * the operation is authorized. Call the normal evidence validator (or session
 * initializer) to determine whether connection authority exists.
 */
gboolean goree_terminal_remote_privacy_adapter_query(
    const GoreeTerminalRemotePrivacyAdapter *adapter,
    const GoreeTerminalRemoteTarget *target,
    gint64 now_unix,
    GoreeTerminalRemotePrivacyEvidence *evidence,
    GError **error);

/* Convenience helper: query current evidence and initialize a remote session. */
gboolean goree_terminal_remote_session_init_from_privacy_adapter(
    GoreeTerminalRemoteSession *session,
    const GoreeTerminalRemoteTarget *target,
    const GoreeTerminalRemotePrivacyAdapter *adapter,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix,
    GError **error);
