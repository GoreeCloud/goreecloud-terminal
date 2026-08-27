#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define GOREE_TERMINAL_GLAZE_VERSION "1.5.0"
#define GOREE_TERMINAL_GLAZE_SOURCE_REVISION "2e1618397f6ebcdd254a76bfdd7e98846f2c5aa3"

typedef enum {
    GOREE_TERMINAL_APPEARANCE_SYSTEM = 0,
    GOREE_TERMINAL_APPEARANCE_LIGHT,
    GOREE_TERMINAL_APPEARANCE_DARK,
} GoreeTerminalAppearance;

GoreeTerminalAppearance goree_terminal_appearance_next(GoreeTerminalAppearance appearance);
const char *goree_terminal_appearance_label(GoreeTerminalAppearance appearance);
const char *goree_terminal_appearance_accessible_label(GoreeTerminalAppearance appearance);

G_END_DECLS
