#include "glaze-contract.h"

GoreeTerminalAppearance
goree_terminal_appearance_next(GoreeTerminalAppearance appearance)
{
    switch (appearance) {
    case GOREE_TERMINAL_APPEARANCE_SYSTEM:
        return GOREE_TERMINAL_APPEARANCE_LIGHT;
    case GOREE_TERMINAL_APPEARANCE_LIGHT:
        return GOREE_TERMINAL_APPEARANCE_DARK;
    case GOREE_TERMINAL_APPEARANCE_DARK:
    default:
        return GOREE_TERMINAL_APPEARANCE_SYSTEM;
    }
}

const char *
goree_terminal_appearance_label(GoreeTerminalAppearance appearance)
{
    switch (appearance) {
    case GOREE_TERMINAL_APPEARANCE_LIGHT:
        return "Light";
    case GOREE_TERMINAL_APPEARANCE_DARK:
        return "Dark";
    case GOREE_TERMINAL_APPEARANCE_SYSTEM:
    default:
        return "System";
    }
}

const char *
goree_terminal_appearance_accessible_label(GoreeTerminalAppearance appearance)
{
    switch (appearance) {
    case GOREE_TERMINAL_APPEARANCE_LIGHT:
        return "Appearance: Light. Activate for Dark.";
    case GOREE_TERMINAL_APPEARANCE_DARK:
        return "Appearance: Dark. Activate for System.";
    case GOREE_TERMINAL_APPEARANCE_SYSTEM:
    default:
        return "Appearance: System. Activate for Light.";
    }
}
