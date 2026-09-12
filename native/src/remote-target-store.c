#include "remote-target-store.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <string.h>

#define REMOTE_TARGET_FILE_NAME "remote-targets.ini"
#define REMOTE_TARGET_GROUP_PREFIX "RemoteTarget:"

static char *remote_targets_path = NULL;

static void
set_validation_error(GError **error, const char *message)
{
    g_set_error_literal(
        error,
        G_OPTION_ERROR,
        G_OPTION_ERROR_BAD_VALUE,
        message);
}

static const char *
resolve_remote_targets_path(void)
{
    const char *override = g_getenv("GOREE_TERMINAL_REMOTE_TARGETS_PATH");
    if (override != NULL && *override != '\0')
        return override;

    if (remote_targets_path == NULL) {
        remote_targets_path = g_build_filename(
            g_get_user_config_dir(),
            "goreecloud",
            "terminal",
            REMOTE_TARGET_FILE_NAME,
            NULL);
    }
    return remote_targets_path;
}

const char *
goree_terminal_remote_targets_path(void)
{
    return resolve_remote_targets_path();
}

static gboolean
valid_record_id(const char *id)
{
    if (id == NULL || *id == '\0' || strlen(id) > GOREE_TERMINAL_REMOTE_TARGET_ID_MAX)
        return FALSE;

    for (const char *cursor = id; *cursor != '\0'; cursor++) {
        if (!(g_ascii_islower(*cursor) || g_ascii_isdigit(*cursor) ||
              *cursor == '-' || *cursor == '_'))
            return FALSE;
    }
    return TRUE;
}

static gboolean
valid_record_name(const char *name)
{
    return name != NULL && *name != '\0' &&
           g_utf8_validate(name, -1, NULL) &&
           g_utf8_strlen(name, -1) <= GOREE_TERMINAL_REMOTE_TARGET_NAME_MAX;
}

gboolean
goree_terminal_remote_target_record_validate(
    const GoreeTerminalRemoteTargetRecord *record,
    GError **error)
{
    g_return_val_if_fail(record != NULL, FALSE);

    if (!valid_record_id(record->id)) {
        set_validation_error(
            error,
            "Remote target ID must use lowercase letters, digits, '-' or '_'.");
        return FALSE;
    }
    if (!valid_record_name(record->name)) {
        set_validation_error(error, "Remote target name is missing, invalid, or too long.");
        return FALSE;
    }

    GoreeTerminalRemoteTarget validated;
    return goree_terminal_remote_target_init(
        &validated,
        record->target.host,
        record->target.username,
        record->target.port,
        error);
}

GoreeTerminalRemoteTargetRecord *
goree_terminal_remote_target_record_new(
    const char *id,
    const char *name,
    const char *host,
    const char *username,
    guint port,
    GError **error)
{
    GoreeTerminalRemoteTargetRecord *record = g_new0(
        GoreeTerminalRemoteTargetRecord,
        1);
    record->id = g_strdup(id);
    record->name = g_strdup(name);

    if (!goree_terminal_remote_target_init(
            &record->target,
            host,
            username,
            port,
            error) ||
        !goree_terminal_remote_target_record_validate(record, error)) {
        goree_terminal_remote_target_record_free(record);
        return NULL;
    }
    return record;
}

GoreeTerminalRemoteTargetRecord *
goree_terminal_remote_target_record_copy(
    const GoreeTerminalRemoteTargetRecord *record)
{
    g_return_val_if_fail(record != NULL, NULL);

    GoreeTerminalRemoteTargetRecord *copy = g_new0(
        GoreeTerminalRemoteTargetRecord,
        1);
    copy->id = g_strdup(record->id);
    copy->name = g_strdup(record->name);
    copy->target = record->target;
    return copy;
}

void
goree_terminal_remote_target_record_free(GoreeTerminalRemoteTargetRecord *record)
{
    if (record == NULL)
        return;
    g_free(record->id);
    g_free(record->name);
    memset(&record->target, 0, sizeof(record->target));
    g_free(record);
}

static gboolean
allowed_key(const char *key)
{
    return g_str_equal(key, "name") ||
           g_str_equal(key, "host") ||
           g_str_equal(key, "username") ||
           g_str_equal(key, "port");
}

static GoreeTerminalRemoteTargetRecord *
load_record(GKeyFile *key_file, const char *group, GError **error)
{
    gsize key_count = 0;
    char **keys = g_key_file_get_keys(key_file, group, &key_count, error);
    if (keys == NULL)
        return NULL;

    for (gsize index = 0; index < key_count; index++) {
        if (!allowed_key(keys[index])) {
            g_strfreev(keys);
            set_validation_error(
                error,
                "Remote target metadata contains an unsupported field; credentials, commands, key paths, and connection overrides are not stored here.");
            return NULL;
        }
    }
    g_strfreev(keys);

    const char *id = group + strlen(REMOTE_TARGET_GROUP_PREFIX);
    char *name = g_key_file_get_string(key_file, group, "name", error);
    if (name == NULL)
        return NULL;
    char *host = g_key_file_get_string(key_file, group, "host", error);
    if (host == NULL) {
        g_free(name);
        return NULL;
    }
    char *username = g_key_file_get_string(key_file, group, "username", NULL);
    if (username == NULL)
        username = g_strdup("");

    GError *port_error = NULL;
    guint64 port_value = g_key_file_get_uint64(key_file, group, "port", &port_error);
    if (port_error != NULL) {
        if (g_error_matches(port_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
            g_clear_error(&port_error);
            port_value = 22;
        } else {
            g_propagate_error(error, port_error);
            g_free(name);
            g_free(host);
            g_free(username);
            return NULL;
        }
    }

    if (port_value > G_MAXUINT) {
        set_validation_error(error, "Remote target port is outside the supported range.");
        g_free(name);
        g_free(host);
        g_free(username);
        return NULL;
    }

    GoreeTerminalRemoteTargetRecord *record = goree_terminal_remote_target_record_new(
        id,
        name,
        host,
        username,
        (guint) port_value,
        error);
    g_free(name);
    g_free(host);
    g_free(username);
    return record;
}

GPtrArray *
goree_terminal_remote_targets_load(GError **error)
{
    GPtrArray *records = g_ptr_array_new_with_free_func(
        (GDestroyNotify) goree_terminal_remote_target_record_free);
    GKeyFile *key_file = g_key_file_new();
    GError *local_error = NULL;

    if (!g_key_file_load_from_file(
            key_file,
            resolve_remote_targets_path(),
            G_KEY_FILE_NONE,
            &local_error)) {
        if (g_error_matches(local_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_clear_error(&local_error);
            g_key_file_unref(key_file);
            return records;
        }
        g_propagate_error(error, local_error);
        g_key_file_unref(key_file);
        g_ptr_array_unref(records);
        return NULL;
    }

    GHashTable *ids = g_hash_table_new(g_str_hash, g_str_equal);
    gsize group_count = 0;
    char **groups = g_key_file_get_groups(key_file, &group_count);
    for (gsize index = 0; index < group_count; index++) {
        if (!g_str_has_prefix(groups[index], REMOTE_TARGET_GROUP_PREFIX))
            continue;

        GoreeTerminalRemoteTargetRecord *record = load_record(
            key_file,
            groups[index],
            error);
        if (record == NULL)
            goto failure;
        if (g_hash_table_contains(ids, record->id)) {
            goree_terminal_remote_target_record_free(record);
            set_validation_error(error, "Remote target IDs must be unique.");
            goto failure;
        }
        g_hash_table_add(ids, record->id);
        g_ptr_array_add(records, record);
    }

    g_strfreev(groups);
    g_hash_table_unref(ids);
    g_key_file_unref(key_file);
    return records;

failure:
    g_strfreev(groups);
    g_hash_table_unref(ids);
    g_key_file_unref(key_file);
    g_ptr_array_unref(records);
    return NULL;
}

static gboolean
ensure_parent_directory(const char *path, GError **error)
{
    char *directory = g_path_get_dirname(path);
    if (g_mkdir_with_parents(directory, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to create remote-target directory: %s",
            g_strerror(saved_errno));
        g_free(directory);
        return FALSE;
    }
    if (g_chmod(directory, 0700) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to secure remote-target directory: %s",
            g_strerror(saved_errno));
        g_free(directory);
        return FALSE;
    }
    g_free(directory);
    return TRUE;
}

gboolean
goree_terminal_remote_targets_save(
    const GPtrArray *records,
    GError **error)
{
    g_return_val_if_fail(records != NULL, FALSE);

    GHashTable *ids = g_hash_table_new(g_str_hash, g_str_equal);
    GKeyFile *key_file = g_key_file_new();

    for (guint index = 0; index < records->len; index++) {
        const GoreeTerminalRemoteTargetRecord *record = g_ptr_array_index(
            (GPtrArray *) records,
            index);
        if (!goree_terminal_remote_target_record_validate(record, error))
            goto failure;
        if (g_hash_table_contains(ids, record->id)) {
            set_validation_error(error, "Remote target IDs must be unique.");
            goto failure;
        }
        g_hash_table_add(ids, record->id);

        char *group = g_strdup_printf("%s%s", REMOTE_TARGET_GROUP_PREFIX, record->id);
        g_key_file_set_string(key_file, group, "name", record->name);
        g_key_file_set_string(key_file, group, "host", record->target.host);
        g_key_file_set_string(key_file, group, "username", record->target.username);
        g_key_file_set_uint64(key_file, group, "port", record->target.port);
        g_free(group);
    }

    const char *path = resolve_remote_targets_path();
    if (!ensure_parent_directory(path, error))
        goto failure;

    gsize data_length = 0;
    char *data = g_key_file_to_data(key_file, &data_length, NULL);
    gboolean saved = g_file_set_contents(path, data, (gssize) data_length, error);
    g_free(data);

    if (saved && g_chmod(path, 0600) != 0) {
        int saved_errno = errno;
        g_set_error(
            error,
            G_FILE_ERROR,
            g_file_error_from_errno(saved_errno),
            "Unable to secure remote-target file: %s",
            g_strerror(saved_errno));
        saved = FALSE;
    }

    g_hash_table_unref(ids);
    g_key_file_unref(key_file);
    return saved;

failure:
    g_hash_table_unref(ids);
    g_key_file_unref(key_file);
    return FALSE;
}

const GoreeTerminalRemoteTargetRecord *
goree_terminal_remote_targets_find(
    const GPtrArray *records,
    const char *id)
{
    if (records == NULL || id == NULL || *id == '\0')
        return NULL;
    for (guint index = 0; index < records->len; index++) {
        const GoreeTerminalRemoteTargetRecord *record = g_ptr_array_index(
            (GPtrArray *) records,
            index);
        if (record != NULL && g_strcmp0(record->id, id) == 0)
            return record;
    }
    return NULL;
}
