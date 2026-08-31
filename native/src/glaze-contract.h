#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define GOREE_TERMINAL_GLAZE_VERSION "2.1.0"
#define GOREE_TERMINAL_GLAZE_SOURCE_REVISION "c49113eb8b93c267613fdf1bbca1f814495acad7"
#define GOREE_TERMINAL_GLAZE_GENERAL_TARGET_PX 48
#define GOREE_TERMINAL_GLAZE_TOUCH_ASSISTANCE_TARGET_PX 56

typedef enum {
    GOREE_TERMINAL_APPEARANCE_SYSTEM = 0,
    GOREE_TERMINAL_APPEARANCE_LIGHT,
    GOREE_TERMINAL_APPEARANCE_DARK,
} GoreeTerminalAppearance;

GoreeTerminalAppearance goree_terminal_appearance_next(GoreeTerminalAppearance appearance);
const char *goree_terminal_appearance_label(GoreeTerminalAppearance appearance);
const char *goree_terminal_appearance_accessible_label(GoreeTerminalAppearance appearance);

G_END_DECLS
