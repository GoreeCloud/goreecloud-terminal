#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define GOREE_TERMINAL_GLAZE_PRODUCT_LABEL "GLAZE UI V1.3 — Adaptive Resonance"
#define GOREE_TERMINAL_GLAZE_VERSION "1.3.0"
#define GOREE_TERMINAL_GLAZE_TAG "v1.3.0"
#define GOREE_TERMINAL_GLAZE_SOURCE_REVISION "ff34f232f295c9dcb07e4c681f66d4104d0b9323"
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
