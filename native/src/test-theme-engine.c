#include <glib.h>
#include <glib/gstdio.h>

#include "theme-engine.h"

static char *test_directory;
static char *test_settings_path;

static void
reset_persistence(void)
{
    if (test_settings_path != NULL)
        g_unlink(test_settings_path);
}

static void
test_default_theme(void)
{
    reset_persistence();

    GoreeTerminalThemeEngine engine;
    goree_terminal_theme_engine_init(&engine);

    g_assert_cmpint(goree_terminal_theme_engine_get(&engine), ==, GOREE_TERMINAL_THEME_FOLLOW_SYSTEM);
    g_assert_cmpstr(goree_terminal_theme_id(engine.active), ==, "follow-system");
}

static void
test_theme_selection(void)
{
    reset_persistence();

    GoreeTerminalThemeEngine engine;
    goree_terminal_theme_engine_init(&engine);

    g_assert_true(goree_terminal_theme_engine_set(&engine, "deep-dark"));
    g_assert_cmpint(goree_terminal_theme_engine_get(&engine), ==, GOREE_TERMINAL_THEME_DEEP_DARK);
    g_assert_cmpstr(goree_terminal_theme_label(engine.active), ==, "Deep Dark");
    g_assert_false(goree_terminal_theme_engine_set(&engine, "not-a-theme"));
    g_assert_cmpint(goree_terminal_theme_engine_get(&engine), ==, GOREE_TERMINAL_THEME_DEEP_DARK);
}

static void
test_system_resolution(void)
{
    const GoreeTerminalThemeDefinition *light = goree_terminal_theme_resolve(
        GOREE_TERMINAL_THEME_FOLLOW_SYSTEM,
        FALSE);
    const GoreeTerminalThemeDefinition *dark = goree_terminal_theme_resolve(
        GOREE_TERMINAL_THEME_FOLLOW_SYSTEM,
        TRUE);

    g_assert_cmpstr(light->id, ==, "light");
    g_assert_cmpstr(dark->id, ==, "dark");
    g_assert_false(light->prefers_dark);
    g_assert_true(dark->prefers_dark);
}

static void
test_glaze_personalization_surface(void)
{
    g_assert_cmpstr(goree_terminal_theme_id(GOREE_TERMINAL_THEME_LIGHT), ==, "light");
    g_assert_cmpstr(goree_terminal_theme_id(GOREE_TERMINAL_THEME_DARK), ==, "dark");
    g_assert_cmpstr(goree_terminal_theme_id(GOREE_TERMINAL_THEME_DEEP_DARK), ==, "deep-dark");

    const GoreeTerminalThemeDefinition *deep = goree_terminal_theme_definition(
        GOREE_TERMINAL_THEME_DEEP_DARK);
    g_assert_cmpstr(deep->cursor, ==, "#68AEE0");
    g_assert_nonnull(deep->foreground);
    g_assert_nonnull(deep->background);
}

static void
test_theme_persistence(void)
{
    reset_persistence();

    GoreeTerminalThemeEngine first;
    goree_terminal_theme_engine_init(&first);
    g_assert_true(goree_terminal_theme_engine_set(&first, "deep-dark"));
    g_assert_true(g_file_test(test_settings_path, G_FILE_TEST_IS_REGULAR));

    GoreeTerminalThemeEngine second;
    goree_terminal_theme_engine_init(&second);
    g_assert_cmpint(
        goree_terminal_theme_engine_get(&second),
        ==,
        GOREE_TERMINAL_THEME_DEEP_DARK);

    gchar *contents = NULL;
    gsize length = 0;
    g_assert_true(g_file_get_contents(test_settings_path, &contents, &length, NULL));
    g_assert_nonnull(g_strstr_len(contents, (gssize) length, "mode=deep-dark"));
    g_assert_null(g_strstr_len(contents, (gssize) length, "command"));
    g_assert_null(g_strstr_len(contents, (gssize) length, "history"));
    g_free(contents);
}

static void
test_invalid_persistence_fails_safe(void)
{
    reset_persistence();
    g_assert_true(g_file_set_contents(
        test_settings_path,
        "[Theme]\nmode=not-a-theme\n",
        -1,
        NULL));

    g_test_expect_message(
        G_LOG_DOMAIN,
        G_LOG_LEVEL_WARNING,
        "*theme preference is invalid*Follow System*");

    GoreeTerminalThemeEngine engine;
    goree_terminal_theme_engine_init(&engine);

    g_test_assert_expected_messages();
    g_assert_cmpint(
        goree_terminal_theme_engine_get(&engine),
        ==,
        GOREE_TERMINAL_THEME_FOLLOW_SYSTEM);
}

int
main(int argc, char **argv)
{
    GError *error = NULL;
    test_directory = g_dir_make_tmp("goreecloud-terminal-theme-XXXXXX", &error);
    g_assert_no_error(error);
    g_assert_nonnull(test_directory);

    test_settings_path = g_build_filename(test_directory, "theme.ini", NULL);
    g_setenv("GOREE_TERMINAL_THEME_SETTINGS_PATH", test_settings_path, TRUE);

    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/theme/default", test_default_theme);
    g_test_add_func("/theme/selection", test_theme_selection);
    g_test_add_func("/theme/system-resolution", test_system_resolution);
    g_test_add_func("/theme/glaze-personalization-surface", test_glaze_personalization_surface);
    g_test_add_func("/theme/persistence", test_theme_persistence);
    g_test_add_func("/theme/invalid-persistence-fails-safe", test_invalid_persistence_fails_safe);

    int result = g_test_run();

    reset_persistence();
    g_unsetenv("GOREE_TERMINAL_THEME_SETTINGS_PATH");
    g_free(test_settings_path);
    g_rmdir(test_directory);
    g_free(test_directory);
    return result;
}
