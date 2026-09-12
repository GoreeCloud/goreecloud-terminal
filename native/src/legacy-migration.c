#include "legacy-migration.h"

#include <errno.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <string.h>

#include "session-profile.h"
#include "terminal-preferences.h"
#include "workspace-store.h"

#define MIGRATION_ROOT_SCHEMA "com.goreecloud.Terminal.Migration.LegacyRoot"
#define MIGRATION_PROFILE_SCHEMA "com.goreecloud.Terminal.Migration.LegacyProfile"
#define MIGRATION_MANIFEST "manifest.ini"
#define MIGRATION_VERSION 1
#define PROFILE_NAME_MAX 128
#define PROFILE_ID_MAX 64

static const char *
legacy_root_path(GoreeTerminalLegacyIdentity identity)
{
    return identity == GOREE_TERMINAL_LEGACY_DEVELOPMENT
        ? "/com/goreecloud/Terminal/Devel/"
        : "/com/goreecloud/Terminal/";
}

static void
set_migration_error(GError **error, const char *message)
{
    g_set_error_literal(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, message);
}

void
goree_terminal_migration_report_clear(GoreeTerminalMigrationReport *report)
{
    if (report == NULL)
        return;
    g_clear_pointer(&report->backup_directory, g_free);
    memset(report, 0, sizeof(*report));
}

static gboolean
valid_native_id(const char *id)
{
    if (id == NULL || *id == '\0' || strlen(id) > PROFILE_ID_MAX)
        return FALSE;

    for (const char *cursor = id; *cursor != '\0'; cursor++) {
        if (!(g_ascii_islower(*cursor) || g_ascii_isdigit(*cursor) ||
              *cursor == '-' || *cursor == '_'))
            return FALSE;
    }
    return TRUE;
}

static GSettings *
new_migration_settings(const char *schema_id, const char *path, GError **error)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    GSettingsSchema *schema;
    GSettings *settings;

    if (source == NULL) {
        set_migration_error(error, "No GSettings schema source is available for migration.");
        return NULL;
    }

    schema = g_settings_schema_source_lookup(source, schema_id, TRUE);
    if (schema == NULL) {
        g_set_error(
            error,
            G_OPTION_ERROR,
            G_OPTION_ERROR_FAILED,
            "Required GoreeCloud Terminal migration schema is unavailable: %s",
            schema_id);
        return NULL;
    }

    settings = g_settings_new_full(schema, NULL, path);
    g_settings_schema_unref(schema);
    return settings;
}

static gboolean
ensure_parent_directory(const char *path, GError **error)
{
    char *directory = g_path_get_dirname(path);
    gboolean ok = TRUE;

    if (g_mkdir_with_parents(directory, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to create migration target directory: %s",
            g_strerror(saved_errno));
        ok = FALSE;
    }

    g_free(directory);
    return ok;
}

static gboolean
write_private_file(const char *path, const char *data, gsize length, GError **error)
{
    if (!ensure_parent_directory(path, error))
        return FALSE;
    if (!g_file_set_contents(path, data, (gssize) length, error))
        return FALSE;
    if (g_chmod(path, 0600) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to secure migration file %s: %s",
            path,
            g_strerror(saved_errno));
        return FALSE;
    }
    return TRUE;
}

static gboolean
backup_one(
    GKeyFile *manifest,
    const char *key,
    const char *source_path,
    const char *backup_directory,
    const char *backup_name,
    GError **error)
{
    gboolean existed = g_file_test(source_path, G_FILE_TEST_EXISTS);
    g_key_file_set_boolean(manifest, "Migration", key, existed);

    if (!existed)
        return TRUE;

    char *data = NULL;
    gsize length = 0;
    if (!g_file_get_contents(source_path, &data, &length, error))
        return FALSE;

    char *backup_path = g_build_filename(backup_directory, backup_name, NULL);
    gboolean ok = write_private_file(backup_path, data, length, error);
    g_free(backup_path);
    g_free(data);
    return ok;
}

static char *
create_backup(GError **error)
{
    const char *override = g_getenv("GOREE_TERMINAL_MIGRATION_BACKUP_ROOT");
    char *root = override != NULL && *override != '\0'
        ? g_strdup(override)
        : g_build_filename(
            g_get_user_state_dir(),
            "goreecloud",
            "terminal",
            "migrations",
            NULL);

    if (g_mkdir_with_parents(root, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to create migration backup root: %s",
            g_strerror(saved_errno));
        g_free(root);
        return NULL;
    }

    char *template = g_build_filename(root, "transitional-XXXXXX", NULL);
    g_free(root);
    if (g_mkdtemp(template) == NULL) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to create migration backup directory: %s",
            g_strerror(saved_errno));
        g_free(template);
        return NULL;
    }
    if (g_chmod(template, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to secure migration backup directory: %s",
            g_strerror(saved_errno));
        g_free(template);
        return NULL;
    }
    return template;
}

static char *
backup_native_files(GError **error)
{
    char *backup_directory = create_backup(error);
    if (backup_directory == NULL)
        return NULL;

    GKeyFile *manifest = g_key_file_new();
    g_key_file_set_integer(manifest, "Migration", "version", MIGRATION_VERSION);

    if (!backup_one(
            manifest,
            "preferences-existed",
            goree_terminal_preferences_path(),
            backup_directory,
            "preferences.ini",
            error) ||
        !backup_one(
            manifest,
            "profiles-existed",
            goree_terminal_session_profiles_path(),
            backup_directory,
            "profiles.ini",
            error) ||
        !backup_one(
            manifest,
            "workspaces-existed",
            goree_terminal_workspaces_path(),
            backup_directory,
            "workspaces.ini",
            error)) {
        g_key_file_unref(manifest);
        g_free(backup_directory);
        return NULL;
    }

    gsize length = 0;
    char *data = g_key_file_to_data(manifest, &length, NULL);
    char *manifest_path = g_build_filename(
        backup_directory,
        MIGRATION_MANIFEST,
        NULL);
    gboolean ok = write_private_file(manifest_path, data, length, error);

    g_free(manifest_path);
    g_free(data);
    g_key_file_unref(manifest);

    if (!ok) {
        g_free(backup_directory);
        return NULL;
    }
    return backup_directory;
}

static gboolean
restore_one(
    GKeyFile *manifest,
    const char *key,
    const char *target_path,
    const char *backup_directory,
    const char *backup_name,
    GError **error)
{
    GError *manifest_error = NULL;
    gboolean existed = g_key_file_get_boolean(
        manifest,
        "Migration",
        key,
        &manifest_error);
    if (manifest_error != NULL) {
        g_propagate_error(error, manifest_error);
        return FALSE;
    }

    if (!existed) {
        if (g_remove(target_path) != 0 && errno != ENOENT) {
            int saved_errno = errno;
            g_set_error(
                error,
                G_FILE_ERROR,
                g_file_error_from_errno(saved_errno),
                "Unable to remove migration-created file %s: %s",
                target_path,
                g_strerror(saved_errno));
            return FALSE;
        }
        return TRUE;
    }

    char *backup_path = g_build_filename(backup_directory, backup_name, NULL);
    char *data = NULL;
    gsize length = 0;
    gboolean ok = g_file_get_contents(backup_path, &data, &length, error);
    if (ok)
        ok = write_private_file(target_path, data, length, error);

    g_free(data);
    g_free(backup_path);
    return ok;
}

gboolean
goree_terminal_legacy_rollback(
    const char *backup_directory,
    GError **error)
{
    g_return_val_if_fail(backup_directory != NULL, FALSE);

    char *manifest_path = g_build_filename(
        backup_directory,
        MIGRATION_MANIFEST,
        NULL);
    GKeyFile *manifest = g_key_file_new();
    gboolean loaded = g_key_file_load_from_file(
        manifest,
        manifest_path,
        G_KEY_FILE_NONE,
        error);
    g_free(manifest_path);
    if (!loaded) {
        g_key_file_unref(manifest);
        return FALSE;
    }

    GError *version_error = NULL;
    gint version = g_key_file_get_integer(
        manifest,
        "Migration",
        "version",
        &version_error);
    if (version_error != NULL) {
        g_propagate_error(error, version_error);
        g_key_file_unref(manifest);
        return FALSE;
    }
    if (version != MIGRATION_VERSION) {
        set_migration_error(error, "Migration backup version is unsupported.");
        g_key_file_unref(manifest);
        return FALSE;
    }

    gboolean ok =
        restore_one(
            manifest,
            "preferences-existed",
            goree_terminal_preferences_path(),
            backup_directory,
            "preferences.ini",
            error) &&
        restore_one(
            manifest,
            "profiles-existed",
            goree_terminal_session_profiles_path(),
            backup_directory,
            "profiles.ini",
            error) &&
        restore_one(
            manifest,
            "workspaces-existed",
            goree_terminal_workspaces_path(),
            backup_directory,
            "workspaces.ini",
            error);

    g_key_file_unref(manifest);
    return ok;
}

static gboolean
native_state_exists(void)
{
    return g_file_test(goree_terminal_preferences_path(), G_FILE_TEST_EXISTS) ||
           g_file_test(goree_terminal_session_profiles_path(), G_FILE_TEST_EXISTS) ||
           g_file_test(goree_terminal_workspaces_path(), G_FILE_TEST_EXISTS);
}

static char *
next_import_id(guint index, GHashTable *used_ids)
{
    for (guint candidate = index + 1; candidate < G_MAXUINT; candidate++) {
        char *id = g_strdup_printf("imported-%u", candidate);
        if (!g_hash_table_contains(used_ids, id))
            return id;
        g_free(id);
    }
    return NULL;
}

static gboolean
profile_name_is_supported(const char *name)
{
    return name != NULL && *name != '\0' &&
           g_utf8_validate(name, -1, NULL) &&
           g_utf8_strlen(name, -1) <= PROFILE_NAME_MAX;
}

static GoreeTerminalSessionProfile *
read_legacy_profile(
    const char *root_path,
    const char *source_id,
    gboolean is_default,
    guint ordinal,
    GHashTable *used_ids,
    GoreeTerminalMigrationReport *report,
    GError **error)
{
    char *profile_path = g_strdup_printf(
        "%sProfiles/%s/",
        root_path,
        source_id);
    GSettings *settings = new_migration_settings(
        MIGRATION_PROFILE_SCHEMA,
        profile_path,
        error);
    g_free(profile_path);
    if (settings == NULL)
        return NULL;

    gboolean use_custom_command = g_settings_get_boolean(
        settings,
        "use-custom-command");
    gboolean limit_scrollback = g_settings_get_boolean(
        settings,
        "limit-scrollback");

    if (use_custom_command || !limit_scrollback) {
        g_object_unref(settings);
        if (is_default) {
            set_migration_error(
                error,
                use_custom_command
                    ? "The transitional default profile uses a custom command, which native migration intentionally does not read or import."
                    : "The transitional default profile has unlimited scrollback, which has no exact native migration mapping.");
            return NULL;
        }
        report->profiles_skipped++;
        return GINT_TO_POINTER(1);
    }

    char *label = g_settings_get_string(settings, "label");
    gint scrollback = g_settings_get_int(settings, "scrollback-lines");
    g_object_unref(settings);

    if (scrollback < 0 || (guint64) scrollback > GOREE_TERMINAL_SCROLLBACK_MAX) {
        g_free(label);
        if (is_default) {
            set_migration_error(
                error,
                "The transitional default profile scrollback exceeds the native supported bound.");
            return NULL;
        }
        report->profiles_skipped++;
        return GINT_TO_POINTER(1);
    }

    GoreeTerminalSessionProfile *profile = goree_terminal_session_profile_new_default();
    g_free(profile->id);
    if (is_default) {
        profile->id = g_strdup("default");
    } else if (valid_native_id(source_id) &&
               !g_str_equal(source_id, "default") &&
               !g_hash_table_contains(used_ids, source_id)) {
        profile->id = g_strdup(source_id);
    } else {
        profile->id = next_import_id(ordinal, used_ids);
    }

    if (profile->id == NULL) {
        g_free(label);
        goree_terminal_session_profile_free(profile);
        set_migration_error(error, "Unable to allocate a unique imported profile ID.");
        return NULL;
    }

    g_free(profile->name);
    if (profile_name_is_supported(label)) {
        profile->name = label;
        label = NULL;
    } else if (label == NULL || *label == '\0') {
        profile->name = g_strdup(is_default ? "Imported Default" : "Imported Profile");
    } else {
        g_free(label);
        goree_terminal_session_profile_free(profile);
        if (is_default) {
            set_migration_error(
                error,
                "The transitional default profile label cannot be represented safely by the native profile format.");
            return NULL;
        }
        report->profiles_skipped++;
        return GINT_TO_POINTER(1);
    }
    g_free(label);

    profile->scrollback_lines = (guint) scrollback;
    if (!goree_terminal_session_profile_validate(profile, error)) {
        goree_terminal_session_profile_free(profile);
        return NULL;
    }

    g_hash_table_add(used_ids, g_strdup(profile->id));
    report->profiles_migrated++;
    return profile;
}

static gboolean
build_migrated_state(
    GoreeTerminalLegacyIdentity identity,
    GoreeTerminalPreferences *preferences,
    GPtrArray **profiles_out,
    GPtrArray **workspaces_out,
    GoreeTerminalMigrationReport *report,
    GError **error)
{
    const char *root_path = legacy_root_path(identity);
    GSettings *root = new_migration_settings(
        MIGRATION_ROOT_SCHEMA,
        root_path,
        error);
    if (root == NULL)
        return FALSE;

    char **source_ids = g_settings_get_strv(root, "profile-uuids");
    char *default_source_id = g_settings_get_string(root, "default-profile-uuid");
    preferences->audible_bell = g_settings_get_boolean(root, "audible-bell");
    report->audible_bell_migrated = TRUE;
    g_object_unref(root);

    guint source_count = g_strv_length(source_ids);
    report->profiles_seen = source_count;
    if (source_count == 0) {
        g_strfreev(source_ids);
        g_free(default_source_id);
        set_migration_error(error, "No transitional GoreeCloud Terminal profiles were found to migrate.");
        return FALSE;
    }

    guint default_index = 0;
    if (default_source_id != NULL && *default_source_id != '\0') {
        gboolean found = FALSE;
        for (guint i = 0; i < source_count; i++) {
            if (g_strcmp0(source_ids[i], default_source_id) == 0) {
                default_index = i;
                found = TRUE;
                break;
            }
        }
        if (!found) {
            g_strfreev(source_ids);
            g_free(default_source_id);
            set_migration_error(error, "The transitional default profile is not present in the profile list.");
            return FALSE;
        }
    }

    GPtrArray *profiles = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_session_profile_free);
    GHashTable *used_ids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GHashTable *seen_source_ids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    for (guint pass = 0; pass < source_count; pass++) {
        guint index = pass == 0 ? default_index : pass - 1;
        if (pass > 0 && index >= default_index)
            index++;
        if (index >= source_count)
            continue;

        const char *source_id = source_ids[index];
        if (source_id == NULL || *source_id == '\0' ||
            g_hash_table_contains(seen_source_ids, source_id)) {
            set_migration_error(error, "The transitional profile list contains an empty or duplicate profile ID.");
            goto failure;
        }
        g_hash_table_add(seen_source_ids, g_strdup(source_id));

        GoreeTerminalSessionProfile *profile = read_legacy_profile(
            root_path,
            source_id,
            index,
            used_ids,
            report,
            error);
        if (profile == NULL)
            goto failure;
        if (profile == GINT_TO_POINTER(1))
            continue;
        g_ptr_array_add(profiles, profile);
    }

    if (profiles->len == 0 ||
        g_strcmp0(((GoreeTerminalSessionProfile *) g_ptr_array_index(profiles, 0))->id,
                  "default") != 0) {
        set_migration_error(error, "Migration did not produce a usable native default profile.");
        goto failure;
    }

    GPtrArray *workspaces = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_workspace_free);
    g_ptr_array_add(workspaces, goree_terminal_workspace_new_default());

    g_hash_table_unref(seen_source_ids);
    g_hash_table_unref(used_ids);
    g_strfreev(source_ids);
    g_free(default_source_id);
    *profiles_out = profiles;
    *workspaces_out = workspaces;
    return TRUE;

failure:
    g_hash_table_unref(seen_source_ids);
    g_hash_table_unref(used_ids);
    g_ptr_array_unref(profiles);
    g_strfreev(source_ids);
    g_free(default_source_id);
    return FALSE;
}

gboolean
goree_terminal_legacy_migrate(
    GoreeTerminalLegacyIdentity identity,
    gboolean replace_native,
    gboolean dry_run,
    GoreeTerminalMigrationReport *report,
    GError **error)
{
    g_return_val_if_fail(report != NULL, FALSE);
    goree_terminal_migration_report_clear(report);

    if (identity != GOREE_TERMINAL_LEGACY_PRODUCTION &&
        identity != GOREE_TERMINAL_LEGACY_DEVELOPMENT) {
        set_migration_error(error, "Unknown transitional GoreeCloud Terminal identity.");
        return FALSE;
    }

    if (native_state_exists() && !replace_native) {
        set_migration_error(
            error,
            "Native GoreeCloud Terminal state already exists; use explicit replacement only after reviewing backup/rollback policy.");
        return FALSE;
    }

    GoreeTerminalPreferences preferences;
    if (replace_native && !goree_terminal_preferences_load(&preferences, error))
        return FALSE;
    if (!replace_native)
        goree_terminal_preferences_init(&preferences);

    GPtrArray *profiles = NULL;
    GPtrArray *workspaces = NULL;
    if (!build_migrated_state(
            identity,
            &preferences,
            &profiles,
            &workspaces,
            report,
            error))
        return FALSE;

    if (dry_run) {
        g_ptr_array_unref(profiles);
        g_ptr_array_unref(workspaces);
        return TRUE;
    }

    char *backup_directory = backup_native_files(error);
    if (backup_directory == NULL) {
        g_ptr_array_unref(profiles);
        g_ptr_array_unref(workspaces);
        return FALSE;
    }

    GError *write_error = NULL;
    gboolean written =
        goree_terminal_preferences_save(&preferences, &write_error) &&
        goree_terminal_session_profiles_save(profiles, &write_error) &&
        goree_terminal_workspaces_save(workspaces, &write_error);

    g_ptr_array_unref(profiles);
    g_ptr_array_unref(workspaces);

    if (!written) {
        GError *rollback_error = NULL;
        if (!goree_terminal_legacy_rollback(backup_directory, &rollback_error)) {
            g_set_error(
                error,
                G_OPTION_ERROR,
                G_OPTION_ERROR_FAILED,
                "Migration write failed (%s) and rollback also failed (%s). Backup remains at %s.",
                write_error != NULL ? write_error->message : "unknown write error",
                rollback_error != NULL ? rollback_error->message : "unknown rollback error",
                backup_directory);
            g_clear_error(&write_error);
            g_clear_error(&rollback_error);
            g_free(backup_directory);
            return FALSE;
        }
        g_propagate_error(error, write_error);
        g_free(backup_directory);
        return FALSE;
    }

    report->backup_directory = backup_directory;
    return TRUE;
}
