#pragma once

#include <glib.h>

#define GOREE_TERMINAL_SCROLLBACK_DEFAULT 10000U
#define GOREE_TERMINAL_SCROLLBACK_MAX 1000000U

typedef enum {
    GOREE_TERMINAL_PASTE_CONFIRM_MULTILINE = 0,
    GOREE_TERMINAL_PASTE_CONFIRM_ALWAYS,
} GoreeTerminalPasteProtection;

typedef struct {
    guint scrollback_lines;
    gboolean audible_bell;
    gboolean allow_hyperlinks;
    gboolean search_wrap_around;
    GoreeTerminalPasteProtection paste_protection;
} GoreeTerminalPreferences;

void goree_terminal_preferences_init(GoreeTerminalPreferences *preferences);

gboolean goree_terminal_preferences_load(
    GoreeTerminalPreferences *preferences,
    GError **error);

gboolean goree_terminal_preferences_save(
    const GoreeTerminalPreferences *preferences,
    GError **error);

const char *goree_terminal_preferences_path(void);

const char *goree_terminal_paste_protection_id(
    GoreeTerminalPasteProtection protection);

gboolean goree_terminal_paste_protection_from_id(
    const char *id,
    GoreeTerminalPasteProtection *protection);
