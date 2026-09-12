#include "remote-openssh.h"

#include <string.h>

gboolean
goree_terminal_remote_openssh_build_argv(
    const GoreeTerminalRemoteTarget *target,
    char ***argv_out,
    GError **error)
{
    g_return_val_if_fail(target != NULL, FALSE);
    g_return_val_if_fail(argv_out != NULL, FALSE);

    *argv_out = NULL;

    GoreeTerminalRemoteTarget validated;
    if (!goree_terminal_remote_target_init(
            &validated,
            target->host,
            target->username,
            target->port,
            error))
        return FALSE;

    GPtrArray *argv = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add(argv, g_strdup("ssh"));

    if (validated.port != 22) {
        g_ptr_array_add(argv, g_strdup("-p"));
        g_ptr_array_add(argv, g_strdup_printf("%u", validated.port));
    }

    if (validated.username[0] != '\0') {
        g_ptr_array_add(argv, g_strdup("-l"));
        g_ptr_array_add(argv, g_strdup(validated.username));
    }

    g_ptr_array_add(argv, g_strdup(validated.host));
    g_ptr_array_add(argv, NULL);
    *argv_out = (char **) g_ptr_array_free(argv, FALSE);
    return TRUE;
}
