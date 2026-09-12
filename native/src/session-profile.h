#pragma once

#include <glib.h>

#include "terminal-preferences.h"

typedef enum {
    GOREE_TERMINAL_ENVIRONMENT_INHERIT_SAFE = 0,
    GOREE_TERMINAL_ENVIRONMENT_CLEAN,
} GoreeTerminalEnvironmentPolicy;

typedef struct {
    char *id;
    char *name;
    char *shell_path;
    char *working_directory;
    char *theme_id;
    guint scrollback_lines;
    GoreeTerminalEnvironmentPolicy environment_policy;
    char **environment_allowlist;
} GoreeTerminalSessionProfile;

GoreeTerminalSessionProfile *goree_terminal_session_profile_new_default(void);
GoreeTerminalSessionProfile *goree_terminal_session_profile_copy(
    const GoreeTerminalSessionProfile *profile);
void goree_terminal_session_profile_free(GoreeTerminalSessionProfile *profile);

gboolean goree_terminal_session_profile_validate(
    const GoreeTerminalSessionProfile *profile,
    GError **error);

const char *goree_terminal_environment_policy_id(
    GoreeTerminalEnvironmentPolicy policy);

gboolean goree_terminal_environment_policy_from_id(
    const char *id,
    GoreeTerminalEnvironmentPolicy *policy);

/*
 * Profile persistence stores launch metadata only. Environment values, tokens,
 * passwords, private keys, command history, and terminal contents are outside
 * this contract by design.
 */
GPtrArray *goree_terminal_session_profiles_load(GError **error);

gboolean goree_terminal_session_profiles_save(
    const GPtrArray *profiles,
    GError **error);

const char *goree_terminal_session_profiles_path(void);
