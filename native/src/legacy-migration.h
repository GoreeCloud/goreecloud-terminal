#pragma once

#include <glib.h>

typedef enum {
    GOREE_TERMINAL_LEGACY_PRODUCTION = 0,
    GOREE_TERMINAL_LEGACY_DEVELOPMENT,
} GoreeTerminalLegacyIdentity;

typedef struct {
    guint profiles_seen;
    guint profiles_migrated;
    guint profiles_skipped;
    gboolean audible_bell_migrated;
    char *backup_directory;
} GoreeTerminalMigrationReport;

/* Reports must be zero-initialized before first use. */
void goree_terminal_migration_report_clear(
    GoreeTerminalMigrationReport *report);

/*
 * Explicitly migrates the bounded, non-secret subset of the transitional
 * GoreeCloud Terminal GSettings model into the native stores.
 *
 * Terminal contents/history, custom command contents, credentials, tokens,
 * private keys, SSH secrets, and environment values are never read or copied.
 * The installed maintenance command performs a dry-run preflight and requires
 * explicit --allow-partial authority before it permits skipped secondary
 * profiles to reach a write operation. The transitional source is never
 * removed or modified by this API.
 */
gboolean goree_terminal_legacy_migrate(
    GoreeTerminalLegacyIdentity identity,
    gboolean replace_native,
    gboolean dry_run,
    GoreeTerminalMigrationReport *report,
    GError **error);

/* Restore the native files captured by a migration backup. */
gboolean goree_terminal_legacy_rollback(
    const char *backup_directory,
    GError **error);
