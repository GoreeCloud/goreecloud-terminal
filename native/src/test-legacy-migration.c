#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>

#include "legacy-migration.h"
#include "session-profile.h"
#include "terminal-preferences.h"
#include "workspace-store.h"

#ifndef MIGRATION_SCHEMA_SOURCE
#error MIGRATION_SCHEMA_SOURCE must point to the migration compatibility schema
#endif

#define ROOT_SCHEMA "com.goreecloud.Terminal.Migration.LegacyRoot"
#define PROFILE_SCHEMA "com.goreecloud.Terminal.Migration.LegacyProfile"

static char *test_root;
static char *preferences_path;
static char *profiles_path;
static char *workspaces_path;
static char *backup_root;

static const char *
root_path(GoreeTerminalLegacyIdentity identity)
{
    return identity == GOREE_TERMINAL_LEGACY_DEVELOPMENT
        ? "/com/goreecloud/Terminal/Devel/"
        : "/com/goreecloud/Terminal/";
}

static GSettings *
settings_for(const char *schema_id, const char *path)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    g_assert_nonnull(source);
    GSettingsSchema *schema = g_settings_schema_source_lookup(source, schema_id, TRUE);
    g_assert_nonnull(schema);
    GSettings *settings = g_settings_new_full(schema, NULL, path);
    g_settings_schema_unref(schema);
    return settings;
}

static void
reset_root(GoreeTerminalLegacyIdentity identity)
{
    GSettings *settings = settings_for(ROOT_SCHEMA, root_path(identity));
    g_settings_reset(settings, "default-profile-uuid");
    g_settings_reset(settings, "profile-uuids");
    g_settings_reset(settings, "audible-bell");
    g_object_unref(settings);
}

static void
reset_profile(GoreeTerminalLegacyIdentity identity, const char *id)
{
    char *path = g_strdup_printf("%sProfiles/%s/", root_path(identity), id);
    GSettings *settings = settings_for(PROFILE_SCHEMA, path);
    g_settings_reset(settings, "label");
    g_settings_reset(settings, "limit-scrollback");
    g_settings_reset(settings, "scrollback-lines");
    g_settings_reset(settings, "use-custom-command");
    g_object_unref(settings);
    g_free(path);
}

static void
configure_root(
    GoreeTerminalLegacyIdentity identity,
    const char *const *ids,
    const char *default_id,
    gboolean audible_bell)
{
    reset_root(identity);
    GSettings *settings = settings_for(ROOT_SCHEMA, root_path(identity));
    g_assert_true(g_settings_set_strv(settings, "profile-uuids", ids));
    g_assert_true(g_settings_set_string(settings, "default-profile-uuid", default_id));
    g_assert_true(g_settings_set_boolean(settings, "audible-bell", audible_bell));
    g_object_unref(settings);
}

static void
configure_profile(
    GoreeTerminalLegacyIdentity identity,
    const char *id,
    const char *label,
    gboolean limit_scrollback,
    gint scrollback_lines,
    gboolean use_custom_command)
{
    reset_profile(identity, id);
    char *path = g_strdup_printf("%sProfiles/%s/", root_path(identity), id);
    GSettings *settings = settings_for(PROFILE_SCHEMA, path);
    g_assert_true(g_settings_set_string(settings, "label", label));
    g_assert_true(g_settings_set_boolean(settings, "limit-scrollback", limit_scrollback));
    g_assert_true(g_settings_set_int(settings, "scrollback-lines", scrollback_lines));
    g_assert_true(g_settings_set_boolean(settings, "use-custom-command", use_custom_command));
    g_object_unref(settings);
    g_free(path);
}

static void
remove_native_state(void)
{
    g_remove(preferences_path);
    g_remove(profiles_path);
    g_remove(workspaces_path);
}

static char *
read_file(const char *path)
{
    char *data = NULL;
    gsize length = 0;
    GError *error = NULL;
    g_assert_true(g_file_get_contents(path, &data, &length, &error));
    g_assert_no_error(error);
    return data;
}

static void
test_fresh_migration_and_rollback(void)
{
    remove_native_state();

    const char *ids[] = {"legacy-default", "safe-two", "skip-me", NULL};
    configure_root(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        ids,
        "legacy-default",
        TRUE);
    configure_profile(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        "legacy-default",
        "Migrated Default",
        TRUE,
        32100,
        FALSE);
    configure_profile(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        "safe-two",
        "Operations",
        TRUE,
        5555,
        FALSE);
    configure_profile(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        "skip-me",
        "Custom Command",
        TRUE,
        1000,
        TRUE);

    GoreeTerminalMigrationReport report = {0};
    GError *error = NULL;
    g_assert_true(goree_terminal_legacy_migrate(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        FALSE,
        FALSE,
        &report,
        &error));
    g_assert_no_error(error);
    g_assert_cmpuint(report.profiles_seen, ==, 3);
    g_assert_cmpuint(report.profiles_migrated, ==, 2);
    g_assert_cmpuint(report.profiles_skipped, ==, 1);
    g_assert_true(report.audible_bell_migrated);
    g_assert_nonnull(report.backup_directory);

    GoreeTerminalPreferences preferences;
    g_assert_true(goree_terminal_preferences_load(&preferences, &error));
    g_assert_no_error(error);
    g_assert_true(preferences.audible_bell);
    g_assert_cmpuint(preferences.scrollback_lines, ==, 32100);

    GPtrArray *profiles = goree_terminal_session_profiles_load(&error);
    g_assert_no_error(error);
    g_assert_nonnull(profiles);
    g_assert_cmpuint(profiles->len, ==, 2);
    GoreeTerminalSessionProfile *default_profile = g_ptr_array_index(profiles, 0);
    GoreeTerminalSessionProfile *second_profile = g_ptr_array_index(profiles, 1);
    g_assert_cmpstr(default_profile->id, ==, "default");
    g_assert_cmpstr(default_profile->name, ==, "Migrated Default");
    g_assert_cmpuint(default_profile->scrollback_lines, ==, 32100);
    g_assert_cmpstr(second_profile->id, ==, "safe-two");
    g_assert_cmpstr(second_profile->name, ==, "Operations");
    g_ptr_array_unref(profiles);

    GPtrArray *workspaces = goree_terminal_workspaces_load(&error);
    g_assert_no_error(error);
    g_assert_nonnull(workspaces);
    g_assert_cmpuint(workspaces->len, ==, 1);
    GoreeTerminalWorkspace *workspace = g_ptr_array_index(workspaces, 0);
    GoreeTerminalWorkspaceTab *tab = g_ptr_array_index(workspace->tabs, 0);
    g_assert_cmpstr(g_ptr_array_index(tab->profile_ids, 0), ==, "default");
    g_ptr_array_unref(workspaces);

    char *manifest = g_build_filename(report.backup_directory, "manifest.ini", NULL);
    GStatBuf stat_buffer;
    g_assert_cmpint(g_stat(manifest, &stat_buffer), ==, 0);
    g_assert_cmpint(stat_buffer.st_mode & 0777, ==, 0600);
    g_free(manifest);

    g_assert_true(goree_terminal_legacy_rollback(report.backup_directory, &error));
    g_assert_no_error(error);
    g_assert_false(g_file_test(preferences_path, G_FILE_TEST_EXISTS));
    g_assert_false(g_file_test(profiles_path, G_FILE_TEST_EXISTS));
    g_assert_false(g_file_test(workspaces_path, G_FILE_TEST_EXISTS));

    goree_terminal_migration_report_clear(&report);
}

static void
test_dry_run_and_replace_restore(void)
{
    remove_native_state();

    GoreeTerminalPreferences original;
    goree_terminal_preferences_init(&original);
    original.scrollback_lines = 4444;
    original.audible_bell = FALSE;
    original.allow_hyperlinks = FALSE;
    GError *error = NULL;
    g_assert_true(goree_terminal_preferences_save(&original, &error));
    g_assert_no_error(error);
    char *before = read_file(preferences_path);

    const char *ids[] = {"replace-default", NULL};
    configure_root(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        ids,
        "replace-default",
        TRUE);
    configure_profile(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        "replace-default",
        "Replacement Default",
        TRUE,
        7777,
        FALSE);

    GoreeTerminalMigrationReport dry_report = {0};
    g_assert_true(goree_terminal_legacy_migrate(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        FALSE,
        TRUE,
        &dry_report,
        &error));
    g_assert_no_error(error);
    g_assert_null(dry_report.backup_directory);
    char *after_dry_run = read_file(preferences_path);
    g_assert_cmpstr(before, ==, after_dry_run);
    g_free(after_dry_run);
    goree_terminal_migration_report_clear(&dry_report);

    GoreeTerminalMigrationReport blocked_report = {0};
    g_assert_false(goree_terminal_legacy_migrate(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        FALSE,
        FALSE,
        &blocked_report,
        &error));
    g_assert_nonnull(error);
    g_assert_nonnull(g_strstr_len(error->message, -1, "already exists"));
    g_clear_error(&error);
    goree_terminal_migration_report_clear(&blocked_report);

    GoreeTerminalMigrationReport replace_report = {0};
    g_assert_true(goree_terminal_legacy_migrate(
        GOREE_TERMINAL_LEGACY_PRODUCTION,
        TRUE,
        FALSE,
        &replace_report,
        &error));
    g_assert_no_error(error);
    g_assert_nonnull(replace_report.backup_directory);

    GoreeTerminalPreferences migrated;
    g_assert_true(goree_terminal_preferences_load(&migrated, &error));
    g_assert_no_error(error);
    g_assert_true(migrated.audible_bell);
    g_assert_cmpuint(migrated.scrollback_lines, ==, 7777);
    g_assert_false(migrated.allow_hyperlinks);

    g_assert_true(goree_terminal_legacy_rollback(replace_report.backup_directory, &error));
    g_assert_no_error(error);
    char *restored = read_file(preferences_path);
    g_assert_cmpstr(before, ==, restored);
    g_assert_false(g_file_test(profiles_path, G_FILE_TEST_EXISTS));
    g_assert_false(g_file_test(workspaces_path, G_FILE_TEST_EXISTS));

    g_free(restored);
    g_free(before);
    goree_terminal_migration_report_clear(&replace_report);
    remove_native_state();
}

static void
test_default_custom_command_fails_closed(void)
{
    remove_native_state();

    const char *ids[] = {"custom-default", NULL};
    configure_root(
        GOREE_TERMINAL_LEGACY_DEVELOPMENT,
        ids,
        "custom-default",
        FALSE);
    configure_profile(
        GOREE_TERMINAL_LEGACY_DEVELOPMENT,
        "custom-default",
        "Unsafe Default",
        TRUE,
        10000,
        TRUE);

    GoreeTerminalMigrationReport report = {0};
    GError *error = NULL;
    g_assert_false(goree_terminal_legacy_migrate(
        GOREE_TERMINAL_LEGACY_DEVELOPMENT,
        FALSE,
        TRUE,
        &report,
        &error));
    g_assert_nonnull(error);
    g_assert_nonnull(g_strstr_len(error->message, -1, "custom command"));
    g_assert_null(report.backup_directory);
    g_assert_false(g_file_test(preferences_path, G_FILE_TEST_EXISTS));
    g_assert_false(g_file_test(profiles_path, G_FILE_TEST_EXISTS));
    g_assert_false(g_file_test(workspaces_path, G_FILE_TEST_EXISTS));

    g_clear_error(&error);
    goree_terminal_migration_report_clear(&report);
}

static void
prepare_test_environment(void)
{
    GError *error = NULL;
    char *schema_dir = g_dir_make_tmp("goree-terminal-migration-schema-XXXXXX", &error);
    g_assert_no_error(error);
    g_assert_nonnull(schema_dir);

    char *schema_data = NULL;
    gsize schema_length = 0;
    g_assert_true(g_file_get_contents(
        MIGRATION_SCHEMA_SOURCE,
        &schema_data,
        &schema_length,
        &error));
    g_assert_no_error(error);

    char *schema_copy = g_build_filename(
        schema_dir,
        "com.goreecloud.Terminal.Migration.gschema.xml",
        NULL);
    g_assert_true(g_file_set_contents(
        schema_copy,
        schema_data,
        (gssize) schema_length,
        &error));
    g_assert_no_error(error);
    g_free(schema_copy);
    g_free(schema_data);

    char *compiler = g_find_program_in_path("glib-compile-schemas");
    g_assert_nonnull(compiler);
    char *compiler_argv[] = {
        compiler,
        "--strict",
        schema_dir,
        NULL,
    };
    int wait_status = 0;
    char *standard_error = NULL;
    g_assert_true(g_spawn_sync(
        NULL,
        compiler_argv,
        NULL,
        G_SPAWN_DEFAULT,
        NULL,
        NULL,
        NULL,
        &standard_error,
        &wait_status,
        &error));
    g_assert_no_error(error);
    g_assert_true(g_spawn_check_wait_status(wait_status, &error));
    g_assert_no_error(error);
    g_free(standard_error);
    g_free(compiler);

    g_setenv("GSETTINGS_SCHEMA_DIR", schema_dir, TRUE);
    g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

    test_root = g_dir_make_tmp("goree-terminal-migration-state-XXXXXX", &error);
    g_assert_no_error(error);
    g_assert_nonnull(test_root);
    preferences_path = g_build_filename(test_root, "preferences.ini", NULL);
    profiles_path = g_build_filename(test_root, "profiles.ini", NULL);
    workspaces_path = g_build_filename(test_root, "workspaces.ini", NULL);
    backup_root = g_build_filename(test_root, "backups", NULL);

    g_setenv("GOREE_TERMINAL_PREFERENCES_PATH", preferences_path, TRUE);
    g_setenv("GOREE_TERMINAL_PROFILES_PATH", profiles_path, TRUE);
    g_setenv("GOREE_TERMINAL_WORKSPACES_PATH", workspaces_path, TRUE);
    g_setenv("GOREE_TERMINAL_MIGRATION_BACKUP_ROOT", backup_root, TRUE);

    g_free(schema_dir);
}

int
main(int argc, char **argv)
{
    prepare_test_environment();
    g_test_init(&argc, &argv, NULL);

    g_test_add_func(
        "/goreecloud/terminal/migration/fresh-rollback",
        test_fresh_migration_and_rollback);
    g_test_add_func(
        "/goreecloud/terminal/migration/dry-run-replace-restore",
        test_dry_run_and_replace_restore);
    g_test_add_func(
        "/goreecloud/terminal/migration/custom-command-fail-closed",
        test_default_custom_command_fails_closed);

    int result = g_test_run();
    g_free(backup_root);
    g_free(workspaces_path);
    g_free(profiles_path);
    g_free(preferences_path);
    g_free(test_root);
    return result;
}
