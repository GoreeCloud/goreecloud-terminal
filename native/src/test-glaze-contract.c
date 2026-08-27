#include <glib.h>

#include "glaze-contract.h"

static void
test_stable_identity(void)
{
    g_assert_cmpstr(GOREE_TERMINAL_GLAZE_VERSION, ==, "1.5.0");
    g_assert_cmpstr(
        GOREE_TERMINAL_GLAZE_SOURCE_REVISION,
        ==,
        "2e1618397f6ebcdd254a76bfdd7e98846f2c5aa3");
}

static void
test_appearance_cycle(void)
{
    g_assert_cmpint(
        goree_terminal_appearance_next(GOREE_TERMINAL_APPEARANCE_SYSTEM),
        ==,
        GOREE_TERMINAL_APPEARANCE_LIGHT);
    g_assert_cmpint(
        goree_terminal_appearance_next(GOREE_TERMINAL_APPEARANCE_LIGHT),
        ==,
        GOREE_TERMINAL_APPEARANCE_DARK);
    g_assert_cmpint(
        goree_terminal_appearance_next(GOREE_TERMINAL_APPEARANCE_DARK),
        ==,
        GOREE_TERMINAL_APPEARANCE_SYSTEM);
}

static void
test_accessible_labels(void)
{
    for (GoreeTerminalAppearance appearance = GOREE_TERMINAL_APPEARANCE_SYSTEM;
         appearance <= GOREE_TERMINAL_APPEARANCE_DARK;
         appearance++) {
        g_assert_nonnull(goree_terminal_appearance_label(appearance));
        g_assert_nonnull(goree_terminal_appearance_accessible_label(appearance));
        g_assert_true(
            g_str_has_prefix(
                goree_terminal_appearance_accessible_label(appearance),
                "Appearance: "));
    }
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/native-glaze/stable-identity", test_stable_identity);
    g_test_add_func("/native-glaze/appearance-cycle", test_appearance_cycle);
    g_test_add_func("/native-glaze/accessible-labels", test_accessible_labels);
    return g_test_run();
}
