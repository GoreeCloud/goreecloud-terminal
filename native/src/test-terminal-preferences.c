#include <glib.h>
#include <glib/gstdio.h>

#include "terminal-preferences.h"

static char *test_path = NULL;

static void
reset_file(void)
{
    if (test_path != NULL)
        g_remove(test_path);
}

static void
test_defaults_without_file(void)
{
    GoreeTerminalPreferences preferences;
    GError *error = NULL;

    reset_file();
    g_assert_true(goree_terminal_preferences_load(&preferences, &error));
    g_assert_no_error(error);
    g_assert_cmpuint(preferences.scrollback_lines, ==, GOREE_TERMINAL_SCROLLBACK_DEFAULT);
    g_assert_false(preferences.audible_bell);
    g_assert_true(preferences.allow_hyperlinks);
    g_assert_true(preferences.search_wrap_around);
    g_assert_cmpint(
        preferences.paste_protection,
        ==,
        GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE);
}

static void
test_round_trip(void)
{
    GoreeTerminalPreferences saved;
    GoreeTerminalPreferences loaded;
    GError *error = NULL;

    goree_terminal_preferences_init(&saved);
    saved.scrollback_lines = 42000;
    saved.audible_bell = TRUE;
    saved.allow_hyperlinks = FALSE;
    saved.search_wrap_around = FALSE;
    saved.paste_protection = GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS;

    g_assert_true(goree_terminal_preferences_save(&saved, &error));
    g_assert_no_error(error);
    g_assert_true(goree_terminal_preferences_load(&loaded, &error));
    g_assert_no_error(error);

    g_assert_cmpuint(loaded.scrollback_lines, ==, 42000);
    g_assert_true(loaded.audible_bell);
    g_assert_false(loaded.allow_hyperlinks);
    g_assert_false(loaded.search_wrap_around);
    g_assert_cmpint(loaded.paste_protection, ==, GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS);
}

static void
test_scrollback_is_bounded(void)
{
    GoreeTerminalPreferences saved;
    GoreeTerminalPreferences loaded;
    GError *error = NULL;

    goree_terminal_preferences_init(&saved);
    saved.scrollback_lines = G_MAXUINT;
    g_assert_true(goree_terminal_preferences_save(&saved, &error));
    g_assert_no_error(error);
    g_assert_true(goree_terminal_preferences_load(&loaded, &error));
    g_assert_no_error(error);
    g_assert_cmpuint(loaded.scrollback_lines, ==, GOREE_TERMINAL_SCROLLBACK_MAX);
}

static void
test_paste_protection_ids(void)
{
    GoreeTerminalPasteProtection protection;

    g_assert_true(goree_terminal_paste_protection_from_id("multiline", &protection));
    g_assert_cmpint(protection, ==, GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE);
    g_assert_true(goree_terminal_paste_protection_from_id("always", &protection));
    g_assert_cmpint(protection, ==, GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS);
    g_assert_false(goree_terminal_paste_protection_from_id("disabled", &protection));
}

int
main(int argc, char **argv)
{
    char *directory;

    g_test_init(&argc, &argv, NULL);
    directory = g_dir_make_tmp("goreecloud-terminal-preferences-XXXXXX", NULL);
    g_assert_nonnull(directory);
    test_path = g_build_filename(directory, "preferences.ini", NULL);
    g_setenv("GOREE_TERMINAL_PREFERENCES_PATH", test_path, TRUE);

    g_test_add_func("/preferences/defaults", test_defaults_without_file);
    g_test_add_func("/preferences/round-trip", test_round_trip);
    g_test_add_func("/preferences/scrollback-bound", test_scrollback_is_bounded);
    g_test_add_func("/preferences/paste-protection-ids", test_paste_protection_ids);

    int status = g_test_run();
    reset_file();
    g_rmdir(directory);
    g_free(test_path);
    g_free(directory);
    return status;
}
