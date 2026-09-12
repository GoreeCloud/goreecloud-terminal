#include <glib.h>

#include "paste-guard.h"

static void
test_single_line_default_is_safe(void)
{
    GoreeTerminalPasteAssessment assessment = goree_terminal_assess_paste(
        "printf hello",
        GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE);

    g_assert_cmpint(assessment.decision, ==, GOREE_TERMINAL_PASTE_SAFE);
    g_assert_cmpuint(assessment.line_count, ==, 1);
    g_assert_false(assessment.contains_newline);
    g_assert_false(assessment.contains_control_characters);
}

static void
test_multiline_requires_confirmation(void)
{
    GoreeTerminalPasteAssessment assessment = goree_terminal_assess_paste(
        "first\nsecond",
        GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE);

    g_assert_cmpint(
        assessment.decision,
        ==,
        GOREE_TERMINAL_PASTE_REQUIRES_CONFIRMATION);
    g_assert_cmpuint(assessment.line_count, ==, 2);
    g_assert_true(assessment.contains_newline);
}

static void
test_control_character_requires_confirmation(void)
{
    GoreeTerminalPasteAssessment assessment = goree_terminal_assess_paste(
        "safe\x1b[31m",
        GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE);

    g_assert_cmpint(
        assessment.decision,
        ==,
        GOREE_TERMINAL_PASTE_REQUIRES_CONFIRMATION);
    g_assert_true(assessment.contains_control_characters);
}

static void
test_always_mode_confirms_single_line(void)
{
    GoreeTerminalPasteAssessment assessment = goree_terminal_assess_paste(
        "echo hello",
        GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS);

    g_assert_cmpint(
        assessment.decision,
        ==,
        GOREE_TERMINAL_PASTE_REQUIRES_CONFIRMATION);
}

static void
test_invalid_utf8_is_rejected(void)
{
    const char invalid[] = {(char) 0xff, '\0'};
    GoreeTerminalPasteAssessment assessment = goree_terminal_assess_paste(
        invalid,
        GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE);

    g_assert_cmpint(
        assessment.decision,
        ==,
        GOREE_TERMINAL_PASTE_REJECT_INVALID_TEXT);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/paste/single-line-safe", test_single_line_default_is_safe);
    g_test_add_func("/paste/multiline-confirm", test_multiline_requires_confirmation);
    g_test_add_func("/paste/control-confirm", test_control_character_requires_confirmation);
    g_test_add_func("/paste/always-confirm", test_always_mode_confirms_single_line);
    g_test_add_func("/paste/invalid-utf8", test_invalid_utf8_is_rejected);
    return g_test_run();
}
