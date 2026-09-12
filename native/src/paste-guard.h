#pragma once

#include <glib.h>

#include "terminal-preferences.h"

typedef enum {
    GOREE_TERMINAL_PASTE_SAFE = 0,
    GOREE_TERMINAL_PASTE_REQUIRES_CONFIRMATION,
    GOREE_TERMINAL_PASTE_REJECT_INVALID_TEXT,
} GoreeTerminalPasteDecision;

typedef struct {
    GoreeTerminalPasteDecision decision;
    guint line_count;
    gboolean contains_control_characters;
    gboolean contains_newline;
} GoreeTerminalPasteAssessment;

GoreeTerminalPasteAssessment goree_terminal_assess_paste(
    const char *text,
    GoreeTerminalPasteProtection protection);
