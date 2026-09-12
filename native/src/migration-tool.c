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

int
main(int argc, char **argv)
{
    char *source = NULL;
    char *rollback = NULL;
    gboolean replace_native = FALSE;
    gboolean dry_run = FALSE;

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
        g_option_context_free(context);
        g_free(source);
        g_free(rollback);
        return 2;
    }

    if (argc != 1) {
        g_printerr("goreecloud-terminal-migrate: unexpected positional arguments\n");
        g_option_context_free(context);
        g_free(source);
        g_free(rollback);
        return 2;
    }

    if (rollback != NULL) {
        if (source != NULL || replace_native || dry_run) {
            g_printerr(
                "goreecloud-terminal-migrate: --rollback cannot be combined with migration options\n");
            g_option_context_free(context);
            g_free(source);
            g_free(rollback);
            return 2;
        }

        gboolean ok = goree_terminal_legacy_rollback(rollback, &error);
        if (!ok) {
            g_printerr(
                "goreecloud-terminal-migrate: rollback failed: %s\n",
                error != NULL ? error->message : "unknown error");
            g_clear_error(&error);
            g_option_context_free(context);
            g_free(source);
            g_free(rollback);
            return 1;
        }

        g_print("Rollback completed from %s\n", rollback);
        g_option_context_free(context);
        g_free(source);
        g_free(rollback);
        return 0;
    }

    GoreeTerminalLegacyIdentity identity;
    if (source == NULL || !parse_identity(source, &identity)) {
        g_printerr(
            "goreecloud-terminal-migrate: --source=production or --source=development is required\n");
        g_option_context_free(context);
        g_free(source);
        g_free(rollback);
        return 2;
    }

    GoreeTerminalMigrationReport report = {0};
    gboolean ok = goree_terminal_legacy_migrate(
        identity,
        replace_native,
        dry_run,
        &report,
        &error);
    if (!ok) {
        g_printerr(
            "goreecloud-terminal-migrate: migration failed: %s\n",
            error != NULL ? error->message : "unknown error");
        g_clear_error(&error);
        goree_terminal_migration_report_clear(&report);
        g_option_context_free(context);
        g_free(source);
        g_free(rollback);
        return 1;
    }

    g_print(
        "%s succeeded: profiles seen=%u migrated=%u skipped=%u audible-bell=%s\n",
        dry_run ? "Migration dry run" : "Migration",
        report.profiles_seen,
        report.profiles_migrated,
        report.profiles_skipped,
        report.audible_bell_migrated ? "migrated" : "not-migrated");
    if (report.backup_directory != NULL)
        g_print("Rollback backup: %s\n", report.backup_directory);

    goree_terminal_migration_report_clear(&report);
    g_option_context_free(context);
    g_free(source);
    g_free(rollback);
    return 0;
}
