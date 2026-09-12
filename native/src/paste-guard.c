#include "paste-guard.h"

GoreeTerminalPasteAssessment
goree_terminal_assess_paste(
    const char *text,
    GoreeTerminalPasteProtection protection)
{
    GoreeTerminalPasteAssessment assessment = {
        .decision = GOREE_TERMINAL_PASTE_SAFE,
        .line_count = 1,
        .contains_control_characters = FALSE,
        .contains_newline = FALSE,
    };

    if (text == NULL || !g_utf8_validate(text, -1, NULL)) {
        assessment.decision = GOREE_TERMINAL_PASTE_REJECT_INVALID_TEXT;
        return assessment;
    }

    const guint8 *cursor = (const guint8 *) text;
    while (*cursor != '\0') {
        if (*cursor == '\n' || *cursor == '\r') {
            assessment.contains_newline = TRUE;
            assessment.line_count++;
            if (*cursor == '\r' && cursor[1] == '\n')
                cursor++;
        } else if ((*cursor < 0x20 && *cursor != '\t') || *cursor == 0x7f) {
            assessment.contains_control_characters = TRUE;
        }
        cursor++;
    }

    if (protection == GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS ||
        assessment.contains_newline ||
        assessment.contains_control_characters) {
        assessment.decision = GOREE_TERMINAL_PASTE_REQUIRES_CONFIRMATION;
    }

    return assessment;
}
