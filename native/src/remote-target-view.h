#pragma once

#include <gtk/gtk.h>

#include "remote-target-runtime.h"

GtkWidget *goree_terminal_remote_target_view_new(
    const GPtrArray *catalog,
    const char *load_error);
