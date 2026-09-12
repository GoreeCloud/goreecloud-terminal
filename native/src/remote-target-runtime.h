#pragma once

#include <glib.h>

#include "remote-privacy-adapter.h"
#include "remote-target-store.h"

typedef struct {
    GoreeTerminalRemoteTargetRecord *record;
    GoreeTerminalRemoteSession session;
} GoreeTerminalRemoteTargetStatus;

GoreeTerminalRemoteTargetStatus *goree_terminal_remote_target_status_new(
    const GoreeTerminalRemoteTargetRecord *record,
    const GoreeTerminalRemotePrivacyAdapter *privacy_adapter,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix,
    GError **error);

void goree_terminal_remote_target_status_free(
    GoreeTerminalRemoteTargetStatus *status);

GPtrArray *goree_terminal_remote_target_catalog_load(
    const GoreeTerminalRemotePrivacyAdapter *privacy_adapter,
    const GoreeTerminalRemoteSecurityContext *security,
    gint64 now_unix,
    GError **error);

const GoreeTerminalRemoteTargetStatus *goree_terminal_remote_target_catalog_find(
    const GPtrArray *catalog,
    const char *id);

const char *goree_terminal_remote_target_status_label(
    const GoreeTerminalRemoteTargetStatus *status);
