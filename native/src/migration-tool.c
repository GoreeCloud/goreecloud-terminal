#include <glib.h>

#include "legacy-migration.h"

static gboolean
parse_identity(const char *source, GoreeTerminalLegacyIdentity *identity)
{
    if (g_strcmp0(source, "production") == 0) {
        *identity = GOREE_TERMINAL_LEGACY_PRODUCTION;
        return TRUE;
    }
    if (g_strcmp0(source, "development") == 0) {
        *identity = GOREE_TERMINAL_LEGACY_DEVELOPMENT;
        return TRUE;
    }
    return FALSE;
}

static void
print_report(const char *label, const GoreeTerminalMigrationReport *report)
{
    g_print(
        "%s: profiles seen=%u migrated=%u skipped=%u audible-bell=%s\n",
        label,
        report->profiles_seen,
        report->profiles_migrated,
        report->profiles_skipped,
        report->audible_bell_migrated ? "migrated" : "not-migrated");
}

static void
free_options(GOptionContext *context, char *source, char *rollback)
{
    g_option_context_free(context);
    g_free(source);
    g_free(rollback);
}

int
main(int argc, char **argv)
{
    char *source = NULL;
    char *rollback = NULL;
    gboolean replace_native = FALSE;
    gboolean dry_run = FALSE;
    gboolean allow_partial = FALSE;

    GOptionEntry entries[] = {
        {
            "source",
            0,
            0,
            G_OPTION_ARG_STRING,
            &source,
            "Transitional source identity: production or development",
            "IDENTITY",
        },
        {
            "replace-native",
            0,
            0,
            G_OPTION_ARG_NONE,
            &replace_native,
            "Replace existing native state after creating a rollback backup",
            NULL,
        },
        {
            "dry-run",
            0,
            0,
            G_OPTION_ARG_NONE,
            &dry_run,
            "Validate migration without writing native state or creating a backup",
            NULL,
        },
        {
            "allow-partial",
            0,
            0,
            G_OPTION_ARG_NONE,
            &allow_partial,
            "Explicitly permit unsupported non-default profiles to remain only in the untouched transitional source",
            NULL,
        },
        {
            "rollback",
            0,
            0,
            G_OPTION_ARG_FILENAME,
            &rollback,
            "Restore native state from a migration backup directory",
            "DIRECTORY",
        },
        {NULL},
    };

    GOptionContext *context = g_option_context_new(
        "- explicit GoreeCloud Terminal migration and rollback maintenance tool");
    g_option_context_add_main_entries(context, entries, NULL);

    GError *error = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("goreecloud-terminal-migrate: %s\n", error->message);
        g_clear_error(&error);
        free_options(context, source, rollback);
        return 2;
    }

    if (argc != 1) {
        g_printerr("goreecloud-terminal-migrate: unexpected positional arguments\n");
        free_options(context, source, rollback);
        return 2;
    }

    if (rollback != NULL) {
        if (source != NULL || replace_native || dry_run || allow_partial) {
            g_printerr(
                "goreecloud-terminal-migrate: --rollback cannot be combined with migration options\n");
            free_options(context, source, rollback);
            return 2;
        }

        gboolean ok = goree_terminal_legacy_rollback(rollback, &error);
        if (!ok) {
            g_printerr(
                "goreecloud-terminal-migrate: rollback failed: %s\n",
                error != NULL ? error->message : "unknown error");
            g_clear_error(&error);
            free_options(context, source, rollback);
            return 1;
        }

        g_print("Rollback completed from %s\n", rollback);
        free_options(context, source, rollback);
        return 0;
    }

    GoreeTerminalLegacyIdentity identity;
    if (source == NULL || !parse_identity(source, &identity)) {
        g_printerr(
            "goreecloud-terminal-migrate: --source=production or --source=development is required\n");
        free_options(context, source, rollback);
        return 2;
    }

    GoreeTerminalMigrationReport preflight = {0};
    gboolean ok = goree_terminal_legacy_migrate(
        identity,
        replace_native,
        TRUE,
        &preflight,
        &error);
    if (!ok) {
        g_printerr(
            "goreecloud-terminal-migrate: migration preflight failed: %s\n",
            error != NULL ? error->message : "unknown error");
        g_clear_error(&error);
        goree_terminal_migration_report_clear(&preflight);
        free_options(context, source, rollback);
        return 1;
    }

    print_report("Migration preflight", &preflight);
    if (preflight.profiles_skipped > 0 && !allow_partial) {
        g_printerr(
            "goreecloud-terminal-migrate: preflight found %u unsupported non-default profile(s). "
            "No native state was written. Review the report and use --allow-partial only if leaving those profiles in the untouched transitional source is acceptable.\n",
            preflight.profiles_skipped);
        goree_terminal_migration_report_clear(&preflight);
        free_options(context, source, rollback);
        return 1;
    }

    if (dry_run) {
        g_print("Migration dry run succeeded; no native state was written.\n");
        goree_terminal_migration_report_clear(&preflight);
        free_options(context, source, rollback);
        return 0;
    }

    goree_terminal_migration_report_clear(&preflight);

    GoreeTerminalMigrationReport report = {0};
    ok = goree_terminal_legacy_migrate(
        identity,
        replace_native,
        FALSE,
        &report,
        &error);
    if (!ok) {
        g_printerr(
            "goreecloud-terminal-migrate: migration failed: %s\n",
            error != NULL ? error->message : "unknown error");
        g_clear_error(&error);
        goree_terminal_migration_report_clear(&report);
        free_options(context, source, rollback);
        return 1;
    }

    if (report.profiles_skipped > 0 && !allow_partial) {
        GError *rollback_error = NULL;
        gboolean restored = report.backup_directory != NULL &&
            goree_terminal_legacy_rollback(report.backup_directory, &rollback_error);
        g_printerr(
            "goreecloud-terminal-migrate: transitional state changed after preflight and the write would be partial; rollback %s.\n",
            restored ? "completed" : "failed");
        if (rollback_error != NULL) {
            g_printerr(
                "goreecloud-terminal-migrate: rollback error: %s\n",
                rollback_error->message);
            g_clear_error(&rollback_error);
        }
        goree_terminal_migration_report_clear(&report);
        free_options(context, source, rollback);
        return 1;
    }

    print_report("Migration succeeded", &report);
    if (report.backup_directory != NULL)
        g_print("Rollback backup: %s\n", report.backup_directory);

    goree_terminal_migration_report_clear(&report);
    free_options(context, source, rollback);
    return 0;
}
