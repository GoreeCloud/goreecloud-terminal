#include <glib.h>
#include <string.h>

#include "glaze-contract.h"

static void
test_glaze_identity(void)
{
    g_assert_cmpstr(GOREE_TERMINAL_GLAZE_PRODUCT_LABEL, ==, "GLAZE UI V1.3 — Adaptive Resonance");
    g_assert_cmpstr(GOREE_TERMINAL_GLAZE_VERSION, ==, "1.3.0");
    g_assert_cmpstr(GOREE_TERMINAL_GLAZE_TAG, ==, "v1.3.0");
    g_assert_cmpstr(
        GOREE_TERMINAL_GLAZE_SOURCE_REVISION,
        ==,
        "ff34f232f295c9dcb07e4c681f66d4104d0b9323");
    g_assert_cmpint(GOREE_TERMINAL_GLAZE_GENERAL_TARGET_PX, ==, 48);
    g_assert_cmpint(GOREE_TERMINAL_GLAZE_TOUCH_ASSISTANCE_TARGET_PX, ==, 56);
}

static void
test_appearance_cycle(void)
{
    GoreeTerminalAppearance appearance = GOREE_TERMINAL_APPEARANCE_SYSTEM;

    appearance = goree_terminal_appearance_next(appearance);
    g_assert_cmpint(appearance, ==, GOREE_TERMINAL_APPEARANCE_LIGHT);
    appearance = goree_terminal_appearance_next(appearance);
    g_assert_cmpint(appearance, ==, GOREE_TERMINAL_APPEARANCE_DARK);
    appearance = goree_terminal_appearance_next(appearance);
    g_assert_cmpint(appearance, ==, GOREE_TERMINAL_APPEARANCE_SYSTEM);
}

static void
test_accessible_labels(void)
{
    g_assert_nonnull(strstr(
        goree_terminal_appearance_accessible_label(GOREE_TERMINAL_APPEARANCE_SYSTEM),
        "Activate for Light"));
    g_assert_nonnull(strstr(
        goree_terminal_appearance_accessible_label(GOREE_TERMINAL_APPEARANCE_LIGHT),
        "Activate for Dark"));
    g_assert_nonnull(strstr(
        goree_terminal_appearance_accessible_label(GOREE_TERMINAL_APPEARANCE_DARK),
        "Activate for System"));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/native/glaze/identity", test_glaze_identity);
    g_test_add_func("/native/glaze/appearance-cycle", test_appearance_cycle);
    g_test_add_func("/native/glaze/accessible-labels", test_accessible_labels);
    return g_test_run();
}
