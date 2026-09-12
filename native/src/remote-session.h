#pragma once

#include <glib.h>

#define GOREE_TERMINAL_REMOTE_HOST_MAX 255
#define GOREE_TERMINAL_REMOTE_USER_MAX 64
#define GOREE_TERMINAL_REMOTE_PURPOSE "remote_administration"
#define GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX 72

typedef enum {
    GOREE_TERMINAL_REMOTE_PRIVACY_UNAVAILABLE,
    GOREE_TERMINAL_REMOTE_PRIVACY_DENY,
    GOREE_TERMINAL_REMOTE_PRIVACY_REQUIRE_USER_DECISION,
    GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW,
    GOREE_TERMINAL_REMOTE_PRIVACY_ALLOW_WITH_CONSTRAINTS,
} GoreeTerminalRemotePrivacyDecision;

typedef enum {
    GOREE_TERMINAL_REMOTE_COVERAGE_UNKNOWN,
    GOREE_TERMINAL_REMOTE_COVERAGE_NOT_COVERED,
    GOREE_TERMINAL_REMOTE_COVERAGE_PARTIAL,
    GOREE_TERMINAL_REMOTE_COVERAGE_COVERED,
    GOREE_TERMINAL_REMOTE_COVERAGE_STALE,
    GOREE_TERMINAL_REMOTE_COVERAGE_DEGRADED,
} GoreeTerminalRemoteSecurityCoverage;

typedef enum {
    GOREE_TERMINAL_REMOTE_BLOCKED,
    GOREE_TERMINAL_REMOTE_READY,
    GOREE_TERMINAL_REMOTE_CONNECTING,
    GOREE_TERMINAL_REMOTE_HOST_KEY_VERIFICATION,
    GOREE_TERMINAL_REMOTE_AUTHENTICATING,
    GOREE_TERMINAL_REMOTE_RUNNING,
    GOREE_TERMINAL_REMOTE_DISCONNECTED,
    GOREE_TERMINAL_REMOTE_RECONNECTING,
    GOREE_TERMINAL_REMOTE_EXITED,
} GoreeTerminalRemoteState;

typedef struct {
    char host[GOREE_TERMINAL_REMOTE_HOST_MAX + 1];
    char username[GOREE_TERMINAL_REMOTE_USER_MAX + 1];
    guint16 port;
} GoreeTerminalRemoteTarget;

/*
 * Evidence is produced by a Privacy Shield adapter. Terminal does not mint,
 * upgrade, extend, or reinterpret Privacy Shield authority.
 *
 * target_binding is a Terminal-local SHA-256 binding of the exact normalized
 * SSH destination and purpose. It contains no credential material. The adapter
 * must explicitly establish that the external Privacy Shield decision applies
 * to that exact binding before authority_verified may be true.
 */
typedef struct {
    GoreeTerminalRemotePrivacyDecision decision;
    gboolean authority_verified;
    gboolean runtime_accepted;
    gboolean constraints_present;
    gboolean constraints_satisfied;
    gint64 expires_at_unix;
    char target_binding[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX];
    char purpose[64];
} GoreeTerminalRemotePrivacyEvidence;

/*
 * Wardveil coverage is presentation/security evidence only here. It does not
 * create SSH authority and cannot override Privacy Shield or OpenSSH.
 */
typedef struct {
    GoreeTerminalRemoteSecurityCoverage coverage;
    gboolean evidence_valid;
    gboolean runtime_accepted;
} GoreeTerminalRemoteSecurityContext;

typedef struct {
    GoreeTerminalRemoteTarget target;
    GoreeTerminalRemotePrivacyEvidence privacy;
    GoreeTerminalRemoteSecurityContext security;
    GoreeTerminalRemoteState state;
} GoreeTerminalRemoteSession;

gboolean goree_terminal_remote_target_init(
    GoreeTerminalRemoteTarget *target,
    const char *host,
    const char *username,
    guint port,
    GError **error);

gboolean goree_terminal_remote_target_binding(
    const GoreeTerminalRemoteTarget *target,
    char output[GOREE_TERMINAL_REMOTE_TARGET_BINDING_MAX],
    GError **error);

gboolean goree_terminal_remote_privacy_evidence_validate(
    const GoreeTerminalRemoteTarget *target,
    const GoreeTerminalRemotePrivacyEvidence *evidence,
    gint64 now_unix,
    GError **error);

void goree_terminal_remote_session_init(
    GoreeTerminalRemoteSession *session,
    const GoreeTerminalRemoteTarget *target,
    const GoreeTerminalRemotePrivacyEvidence *privacy,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix);

gboolean goree_terminal_remote_session_can_connect(
    const GoreeTerminalRemoteSession *session,
    gint64 now_unix);

gboolean goree_terminal_remote_session_begin_connect(
    GoreeTerminalRemoteSession *session,
    gint64 now_unix);

gboolean goree_terminal_remote_session_mark_host_key_verification(
    GoreeTerminalRemoteSession *session);

gboolean goree_terminal_remote_session_mark_authenticating(
    GoreeTerminalRemoteSession *session);

gboolean goree_terminal_remote_session_mark_running(
    GoreeTerminalRemoteSession *session);

void goree_terminal_remote_session_mark_disconnected(
    GoreeTerminalRemoteSession *session);

gboolean goree_terminal_remote_session_begin_reconnect(
    GoreeTerminalRemoteSession *session,
    const GoreeTerminalRemotePrivacyEvidence *refreshed_privacy,
    gint64 now_unix);

void goree_terminal_remote_session_mark_exited(
    GoreeTerminalRemoteSession *session);

const char *goree_terminal_remote_coverage_label(
    const GoreeTerminalRemoteSecurityContext *security);
