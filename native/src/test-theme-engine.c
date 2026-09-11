#include <glib.h>

#include "theme-engine.h"

static void
test_default_theme(void)
{
    GoreeTerminalThemeEngine engine;
    goree_terminal_theme_engine_init(&engine);

    g_assert_cmpint(goree_terminal_theme_engine_get(&engine), ==, GOREE_TERMINAL_THEME_FOLLOW_SYSTEM);
    g_assert_cmpstr(goree_terminal_theme_id(engine.active), ==, "follow-system");
}

static void
test_theme_selection(void)
{
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

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/theme/default", test_default_theme);
    g_test_add_func("/theme/selection", test_theme_selection);
    g_test_add_func("/theme/system-resolution", test_system_resolution);
    g_test_add_func("/theme/glaze-personalization-surface", test_glaze_personalization_surface);
    return g_test_run();
}
