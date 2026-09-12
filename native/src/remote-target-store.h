#pragma once

#include <glib.h>

#include "remote-session.h"

#define GOREE_TERMINAL_REMOTE_TARGET_ID_MAX 64
#define GOREE_TERMINAL_REMOTE_TARGET_NAME_MAX 128

typedef struct {
    char *id;
    char *name;
    GoreeTerminalRemoteTarget target;
} GoreeTerminalRemoteTargetRecord;

GoreeTerminalRemoteTargetRecord *goree_terminal_remote_target_record_new(
    const char *id,
    const char *name,
    const char *host,
    const char *username,
    guint port,
    GError **error);

GoreeTerminalRemoteTargetRecord *goree_terminal_remote_target_record_copy(
    const GoreeTerminalRemoteTargetRecord *record);

void goree_terminal_remote_target_record_free(
    GoreeTerminalRemoteTargetRecord *record);

gboolean goree_terminal_remote_target_record_validate(
    const GoreeTerminalRemoteTargetRecord *record,
    GError **error);

const char *goree_terminal_remote_targets_path(void);

GPtrArray *goree_terminal_remote_targets_load(GError **error);

gboolean goree_terminal_remote_targets_save(
    const GPtrArray *records,
    GError **error);

const GoreeTerminalRemoteTargetRecord *goree_terminal_remote_targets_find(
    const GPtrArray *records,
    const char *id);
