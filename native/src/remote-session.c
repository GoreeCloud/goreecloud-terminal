#include "remote-session.h"

#include <string.h>

static void
set_remote_error(GError **error, const char *message)
{
    g_set_error_literal(
        error,
        G_OPTION_ERROR,
        G_OPTION_ERROR_BAD_VALUE,
        message);
}

static gboolean
valid_host_character(char value)
{
    return g_ascii_isalnum(value) || value == '.' || value == '-' ||
           value == '_' || value == ':' || value == '[' || value == ']' ||
           value == '%';
}

static gboolean
valid_username_character(char value)
{
    return g_ascii_isalnum(value) || value == '.' || value == '-' ||
           value == '_';
}

static gboolean
validate_bounded_token(
    const char *value,
    gsize maximum_length,
    gboolean (*character_is_valid)(char),
    gboolean allow_empty,
    GError **error,
    const char *invalid_message)
{
    if (value == NULL)
        value = "";

    gsize length = strlen(value);
    if ((!allow_empty && length == 0) || length > maximum_length ||
        (length > 0 && value[0] == '-')) {
        set_remote_error(error, invalid_message);
        return FALSE;
    }

    for (gsize index = 0; index < length; index++) {
        unsigned char byte = (unsigned char) value[index];
        if (byte > 0x7f || !character_is_valid((char) byte)) {
            set_remote_error(error, invalid_message);
            return FALSE;
        }
    }
    return TRUE;
}

gboolean
goree_terminal_remote_target_init(
    GoreeTerminalRemoteTarget *target,
    const char *host,
    const char *username,
    guint port,
    GError **error)
{
    g_return_val_if_fail(target != NULL, FALSE);

    memset(target, 0, sizeof(*target));
    if (!validate_bounded_token(
            host,
            GOREE_TERMINAL_REMOTE_HOST_MAX,
            valid_host_character,
            FALSE,
            error,
            "Remote SSH host is invalid or outside the supported bound."))
        return FALSE;

    if (!validate_bounded_token(
            username,
            GOREE_TERMINAL_REMOTE_USER_MAX,
            valid_username_character,
            TRUE,
            error,
            "Remote SSH username is invalid or outside the supported bound."))
        return FALSE;

    if (port == 0)
        port = 22;
    if (port > 65535) {
        set_remote_error(error, "Remote SSH port must be between 1 and 65535.");
        return FALSE;
    }

    g_strlcpy(target->host, host, sizeof(target->host));
    g_strlcpy(target->username, username != NULL ? username : "", sizeof(target->username));
    target->port = (guint16) port;
    return TRUE;
}

gboolean
goree_terminal_remote_target_binding(
    const GoreeTerminalRemoteTarget *target,
    char output[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX],
    GError **error)
{
    g_return_val_if_fail(target != NULL, FALSE);
    g_return_val_if_fail(output != NULL, FALSE);

    GoreeTerminalRemoteTarget validated;
    if (!goree_terminal_remote_target_init(
            &validated,
            target->host,
            target->username,
            target->port,
            error))
        return FALSE;

    GChecksum *checksum = g_checksum_new(G_CHECKSUM_SHA256);
    if (checksum == NULL) {
        set_remote_error(error, "Unable to create remote target binding.");
        return FALSE;
    }

    static const guint8 separator = 0;
    char port_text[8];
    g_snprintf(port_text, sizeof(port_text), "%u", validated.port);

    g_checksum_update(checksum, (const guchar *) "goreecloud-terminal-ssh-v1", 26);
    g_checksum_update(checksum, &separator, 1);
    g_checksum_update(checksum, (const guchar *) GOREE_TERMINAL_REMOTE_PURPOSE,
                      strlen(GOREE_TERMINAL_REMOTE_PURPOSE));
    g_checksum_update(checksum, &separator, 1);
    g_checksum_update(checksum, (const guchar *) validated.username,
                      strlen(validated.username));
    g_checksum_update(checksum, &separator, 1);
    g_checksum_update(checksum, (const guchar *) validated.host,
                      strlen(validated.host));
    g_checksum_update(checksum, &separator, 1);
    g_checksum_update(checksum, (const guchar *) port_text, strlen(port_text));

    const char *digest = g_checksum_get_string(checksum);
    if (digest == NULL) {
        g_checksum_free(checksum);
        set_remote_error(error, "Unable to finalize remote target binding.");
        return FALSE;
    }

    g_snprintf(output, GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX, "sha256:%s", digest);
    g_checksum_free(checksum);
    return TRUE;
}

gboolean
goree_terminal_remote_privacy_evidence_validate(
    const GoreeTerminalRemoteTarget *target,
    const GoreeTerminalRemotePrivacyEvidence *evidence,
    gint64 now_unix,
    GError **error)
{
    g_return_val_if_fail(target != NULL, FALSE);
    g_return_val_if_fail(evidence != NULL, FALSE);

    if (!evidence->authority_verified) {
        set_remote_error(error, "Privacy Shield authority for the remote operation is not verified.");
        return FALSE;
    }
    if (!evidence->runtime_accepted) {
        set_remote_error(error, "Privacy Shield runtime acceptance is not established for this remote operation.");
        return FALSE;
    }
    if (g_strcmp0(evidence->purpose, GOREE_TERMINAL_REMOTE_PURPOSE) != 0) {
        set_remote_error(error, "Privacy Shield evidence is bound to a different purpose.");
        return FALSE;
    }
    if (evidence->expires_at_unix <= now_unix) {
        set_remote_error(error, "Privacy Shield evidence for the remote operation is expired.");
        return FALSE;
    }

    char expected_binding[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX];
    if (!goree_terminal_remote_target_binding(target, expected_binding, error))
        return FALSE;
    if (g_strcmp0(evidence->target_binding, expected_binding) != 0) {
        set_remote_error(error, "Privacy Shield evidence is not bound to the requested remote destination.");
        return FALSE;
    }

    if (evidence->decision != GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW &&
        evidence->decision != GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW_WITH_CONSTRAINTS) {
        set_remote_error(error, "Privacy Shield did not authorize the remote operation.");
        return FALSE;
    }

    if (evidence->decision == GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW &&
        evidence->constraints_present) {
        set_remote_error(error, "Constrained Privacy Shield evidence must use the constrained decision outcome.");
        return FALSE;
    }

    if (evidence->decision == GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW_WITH_CONSTRAINTS &&
        !evidence->constraints_present) {
        set_remote_error(error, "Privacy Shield constrained authorization is missing its constraints.");
        return FALSE;
    }

    if (evidence->constraints_present && !evidence->constraints_satisfied) {
        set_remote_error(error, "Privacy Shield constraints for the remote operation are not satisfied.");
        return FALSE;
    }

    return TRUE;
}

void
goree_terminal_remote_session_init(
    GoreeTerminalRemoteSession *session,
    const GoreeTerminalRemoteTarget *target,
    const GoreeTerminalRemotePrivacyEvidence *privacy,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix)
{
    g_return_if_fail(session != NULL);
    g_return_if_fail(target != NULL);
    g_return_if_fail(privacy != NULL);

    memset(session, 0, sizeof(*session));
    session->target = *target;
    session->privacy = *privacy;
    if (security != NULL)
        session->security = *security;
    else
        session->security.coverage = GOREE_TERMINAL_REMOTE_COVERAGE_NOT_COVERED;

    session->state = goree_terminal_remote_privacy_evidence_validate(
        target,
        privacy,
        now_unix,
        NULL)
        ? GOREE_TERMINAL_REMOTE_READY
        : GOREE_TERMINAL_REMOTE_BLOCKED;
}

gboolean
goree_terminal_remote_session_can_connect(
    const GoreeTerminalRemoteSession *session,
    gint64 now_unix)
{
    if (session == NULL)
        return FALSE;
    if (session->state != GOREE_TERMINAL_REMOTE_READY &&
        session->state != GOREE_TERMINAL_REMOTE_DISCONNECTED)
        return FALSE;

    return goree_terminal_remote_privacy_evidence_validate(
        &session->target,
        &session->privacy,
        now_unix,
        NULL);
}

gboolean
goree_terminal_remote_session_begin_connect(
    GoreeTerminalRemoteSession *session,
    gint64 now_unix)
{
    if (!goree_terminal_remote_session_can_connect(session, now_unix))
        return FALSE;

    session->state = GOREE_TERMINAL_REMOTE_CONNECTING;
    return TRUE;
}

gboolean
goree_terminal_remote_session_mark_host_key_verification(
    GoreeTerminalRemoteSession *session)
{
    if (session == NULL || session->state != GOREE_TERMINAL_REMOTE_CONNECTING)
        return FALSE;
    session->state = GOREE_TERMINAL_REMOTE_HOST_KEY_VERIFICATION;
    return TRUE;
}

gboolean
goree_terminal_remote_session_mark_authenticating(
    GoreeTerminalRemoteSession *session)
{
    if (session == NULL ||
        (session->state != GOREE_TERMINAL_REMOTE_CONNECTING &&
         session->state != GOREE_TERMINAL_REMOTE_HOST_KEY_VERIFICATION))
        return FALSE;
    session->state = GOREE_TERMINAL_REMOTE_AUTHENTICATING;
    return TRUE;
}

gboolean
goree_terminal_remote_session_mark_running(
    GoreeTerminalRemoteSession *session)
{
    if (session == NULL || session->state != GOREE_TERMINAL_REMOTE_AUTHENTICATING)
        return FALSE;
    session->state = GOREE_TERMINAL_REMOTE_RUNNING;
    return TRUE;
}

void
goree_terminal_remote_session_mark_disconnected(
    GoreeTerminalRemoteSession *session)
{
    if (session == NULL || session->state == GOREE_TERMINAL_REMOTE_EXITED)
        return;
    session->state = GOREE_TERMINAL_REMOTE_DISCONNECTED;
}

gboolean
goree_terminal_remote_session_begin_reconnect(
    GoreeTerminalRemoteSession *session,
    const GoreeTerminalRemotePrivacyEvidence *refreshed_privacy,
    gint64 now_unix)
{
    if (session == NULL || session->state != GOREE_TERMINAL_REMOTE_DISCONNECTED)
        return FALSE;

    if (refreshed_privacy != NULL)
        session->privacy = *refreshed_privacy;

    if (!goree_terminal_remote_privacy_evidence_validate(
            &session->target,
            &session->privacy,
            now_unix,
            NULL)) {
        session->state = GOREE_TERMINAL_REMOTE_BLOCKED;
        return FALSE;
    }

    session->state = GOREE_TERMINAL_REMOTE_RECONNECTING;
    return TRUE;
}

void
goree_terminal_remote_session_mark_exited(
    GoreeTerminalRemoteSession *session)
{
    if (session == NULL)
        return;
    session->state = GOREE_TERMINAL_REMOTE_EXITED;
}

const char *
goree_terminal_remote_coverage_label(
    const GoreeTerminalRemoteSecurityContext *security)
{
    if (security == NULL || !security->evidence_valid)
        return "Wardveil: Unknown";

    switch (security->coverage) {
    case GOREE_TERMINAL_REMOTE_COVERAGE_COVERED:
        return security->runtime_accepted
            ? "Wardveil: Covered"
            : "Wardveil: Coverage unaccepted";
    case GOREE_TERMINAL_REMOTE_COVERAGE_PARTIAL:
        return "Wardveil: Partially covered";
    case GOREE_TERMINAL_REMOTE_COVERAGE_NOT_COVERED:
        return "Wardveil: Not covered";
    case GOREE_TERMINAL_REMOTE_COVERAGE_STALE:
        return "Wardveil: Coverage stale";
    case GOREE_TERMINAL_REMOTE_COVERAGE_DEGRADED:
        return "Wardveil: Coverage degraded";
    case GOREE_TERMINAL_REMOTE_COVERAGE_UNKNOWN:
    default:
        return "Wardveil: Unknown";
    }
}
