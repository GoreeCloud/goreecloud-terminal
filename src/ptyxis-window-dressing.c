/*
 * ptyxis-window-dressing.c
 *
 * Copyright 2023 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <math.h>

#include "gdkhslaprivate.h"

#include "goreecloud-session-context.h"
#include "ptyxis-application.h"
#include "ptyxis-window-dressing.h"

struct _PtyxisWindowDressing
{
  GObject         parent_instance;
  GWeakRef        window_wr;
  PtyxisPalette  *palette;
  GtkCssProvider *css_provider;
  char           *css_class;
  double          opacity;
  guint           queued_update;

  GtkWidget      *session_context_chip;
  GtkImage       *session_context_icon;
  GtkLabel       *session_context_label;
  GSignalGroup   *session_context_tab_signals;
};

enum {
  PROP_0,
  PROP_OPACITY,
  PROP_PALETTE,
  PROP_WINDOW,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (PtyxisWindowDressing, ptyxis_window_dressing, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS];
static guint last_sequence;

static AdwHeaderBar *
ptyxis_window_dressing_find_header_bar (GtkWidget *widget)
{
  GtkWidget *child;

  if (ADW_IS_HEADER_BAR (widget))
    return ADW_HEADER_BAR (widget);

  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      AdwHeaderBar *header_bar = ptyxis_window_dressing_find_header_bar (child);

      if (header_bar != NULL)
        return header_bar;
    }

  return NULL;
}

static void
ptyxis_window_dressing_update_session_context (PtyxisWindowDressing *self)
{
  g_autoptr(PtyxisWindow) window = NULL;
  const GoreeCloudSessionContext *context;
  PtyxisProcessLeaderKind kind = PTYXIS_PROCESS_LEADER_KIND_UNKNOWN;
  PtyxisTab *tab;

  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));

  if (self->session_context_chip == NULL)
    return;

  window = ptyxis_window_dressing_dup_window (self);
  if (window == NULL)
    return;

  tab = ptyxis_window_get_active_tab (window);
  g_signal_group_set_target (self->session_context_tab_signals, tab);

  if (tab != NULL)
    g_object_get (tab,
                  "process-leader-kind", &kind,
                  NULL);

  context = goreecloud_session_context_for_leader_kind (kind);

  gtk_widget_remove_css_class (self->session_context_chip, "session-context-local");
  gtk_widget_remove_css_class (self->session_context_chip, "session-context-remote");
  gtk_widget_remove_css_class (self->session_context_chip, "session-context-container");
  gtk_widget_remove_css_class (self->session_context_chip, "session-context-elevated");
  gtk_widget_add_css_class (self->session_context_chip, context->css_class);

  gtk_label_set_label (self->session_context_label, context->label);
  gtk_image_set_from_icon_name (self->session_context_icon, context->icon_name);
  gtk_widget_set_tooltip_text (self->session_context_chip,
                               context->accessible_description);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->session_context_chip),
                                  GTK_ACCESSIBLE_PROPERTY_LABEL,
                                  context->accessible_description,
                                  -1);

  gtk_widget_set_visible (self->session_context_chip, context->show_indicator);
}

static void
ptyxis_window_dressing_session_context_tab_notify_cb (PtyxisWindowDressing *self,
                                                      GParamSpec           *pspec,
                                                      PtyxisTab            *tab)
{
  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));
  g_assert (PTYXIS_IS_TAB (tab));

  ptyxis_window_dressing_update_session_context (self);
}

static void
ptyxis_window_dressing_active_tab_notify_cb (PtyxisWindowDressing *self,
                                             GParamSpec           *pspec,
                                             PtyxisWindow         *window)
{
  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));
  g_assert (PTYXIS_IS_WINDOW (window));

  ptyxis_window_dressing_update_session_context (self);
}

static void
ptyxis_window_dressing_create_session_context_chip (PtyxisWindowDressing *self,
                                                    PtyxisWindow         *window)
{
  AdwHeaderBar *header_bar;

  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));
  g_assert (PTYXIS_IS_WINDOW (window));

  header_bar = ptyxis_window_dressing_find_header_bar (GTK_WIDGET (window));
  if (header_bar == NULL)
    {
      g_warning ("Unable to locate terminal header bar for Wardveil session context");
      return;
    }

  self->session_context_icon = GTK_IMAGE (g_object_new (GTK_TYPE_IMAGE,
                                                       "accessible-role", GTK_ACCESSIBLE_ROLE_NONE,
                                                       NULL));
  gtk_image_set_pixel_size (self->session_context_icon, 16);

  self->session_context_label = GTK_LABEL (g_object_new (GTK_TYPE_LABEL,
                                                                 "accessible-role", GTK_ACCESSIBLE_ROLE_NONE,
                                                                 NULL));
  gtk_label_set_ellipsize (self->session_context_label, PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars (self->session_context_label, 16);

  self->session_context_chip = g_object_new (GTK_TYPE_BOX,
                                              "accessible-role", GTK_ACCESSIBLE_ROLE_GROUP,
                                              "orientation", GTK_ORIENTATION_HORIZONTAL,
                                              "spacing", 6,
                                              "valign", GTK_ALIGN_CENTER,
                                              NULL);
  gtk_widget_add_css_class (self->session_context_chip, "wardveil-session-context");
  gtk_box_append (GTK_BOX (self->session_context_chip),
                  GTK_WIDGET (self->session_context_icon));
  gtk_box_append (GTK_BOX (self->session_context_chip),
                  GTK_WIDGET (self->session_context_label));
  gtk_widget_set_visible (self->session_context_chip, FALSE);

  adw_header_bar_pack_end (header_bar, self->session_context_chip);

  g_signal_connect_object (window,
                           "notify::active-tab",
                           G_CALLBACK (ptyxis_window_dressing_active_tab_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);

  ptyxis_window_dressing_update_session_context (self);
}

static void
ptyxis_window_dressing_update (PtyxisWindowDressing *self)
{
  g_autoptr(GString) string = NULL;

  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));

  string = g_string_new (NULL);

  /* Wardveil session context is application chrome, not terminal content.
   * It intentionally uses text plus iconography so state is never conveyed by
   * color alone. Local/unknown state stays hidden and therefore makes no
   * protection claim.
   */
  g_string_append (string,
                   ".wardveil-session-context {"
                   " min-height: 24px;"
                   " padding: 2px 9px;"
                   " border-radius: 9999px;"
                   " border: 1px solid alpha(currentColor,.18);"
                   " background-color: alpha(currentColor,.07);"
                   "}\n"
                   ".wardveil-session-context.session-context-remote {"
                   " background-color: alpha(#48b8ff,.14);"
                   " border-color: alpha(#7fd6ff,.34);"
                   "}\n"
                   ".wardveil-session-context.session-context-container {"
                   " background-color: alpha(#f6c85f,.14);"
                   " border-color: alpha(#f6c85f,.34);"
                   "}\n"
                   ".wardveil-session-context.session-context-elevated {"
                   " background-color: alpha(#ff6b7a,.16);"
                   " border-color: alpha(#ff8a96,.42);"
                   " font-weight: 600;"
                   "}\n");

  if (self->palette != NULL)
    {
      PtyxisSettings *settings = ptyxis_application_get_settings (PTYXIS_APPLICATION_DEFAULT);
      AdwStyleManager *style_manager = adw_style_manager_get_default ();
      gboolean dark = adw_style_manager_get_dark (style_manager);
      const PtyxisPaletteFace *face = ptyxis_palette_get_face (self->palette, dark);
      g_autofree char *bg = NULL;
      g_autofree char *fg = NULL;
      g_autofree char *titlebar_bg = NULL;
      g_autofree char *titlebar_fg = NULL;
      g_autofree char *su_fg = NULL;
      g_autofree char *su_bg = NULL;
      g_autofree char *rm_fg = NULL;
      g_autofree char *rm_bg = NULL;
      g_autofree char *bell_fg = NULL;
      g_autofree char *bell_bg = NULL;
      g_autofree char *accent_mix_str = NULL;
      g_autofree char *revealer_bg = NULL;
      char window_alpha_str[G_ASCII_DTOSTR_BUF_SIZE];
      char popover_alpha_str[G_ASCII_DTOSTR_BUF_SIZE];
      gboolean visual_process_leader;
      GdkRGBA accent_mix;
      double window_alpha;
      double popover_alpha;

      /* Force clear any background applied to terminals from distro,
       * theme, or user settings so we can be sure our palettes work.
       *
       * See #241 for details
       */
      g_string_append (string, "vte-terminal { background: none; }\n");

      bg = gdk_rgba_to_string (&face->background);
      fg = gdk_rgba_to_string (&face->foreground);
      titlebar_bg = gdk_rgba_to_string (&face->titlebar_background);
      titlebar_fg = gdk_rgba_to_string (&face->titlebar_foreground);
      rm_fg = gdk_rgba_to_string (&face->scarves[PTYXIS_PALETTE_SCARF_REMOTE].foreground);
      rm_bg = gdk_rgba_to_string (&face->scarves[PTYXIS_PALETTE_SCARF_REMOTE].background);
      su_fg = gdk_rgba_to_string (&face->scarves[PTYXIS_PALETTE_SCARF_SUPERUSER].foreground);
      su_bg = gdk_rgba_to_string (&face->scarves[PTYXIS_PALETTE_SCARF_SUPERUSER].background);
      bell_fg = gdk_rgba_to_string (&face->scarves[PTYXIS_PALETTE_SCARF_VISUAL_BELL].foreground);
      bell_bg = gdk_rgba_to_string (&face->scarves[PTYXIS_PALETTE_SCARF_VISUAL_BELL].background);

      window_alpha = self->opacity;
      popover_alpha = MAX (window_alpha, 0.85);

      g_ascii_dtostr (popover_alpha_str, sizeof popover_alpha_str, popover_alpha);
      g_ascii_dtostr (window_alpha_str, sizeof window_alpha_str, window_alpha);

      revealer_bg = g_strdup_printf ("alpha(mix(%s,%s,.05),%s)", titlebar_bg, titlebar_fg, popover_alpha_str);

      g_string_append_printf (string,
                              "window.%s { color: %s; background-color: alpha(%s, %s); }\n",
                              self->css_class, fg, bg, window_alpha_str);
      g_string_append_printf (string,
                              "window.%s.fullscreen { background-color: %s; }\n",
                              self->css_class, bg);
      g_string_append_printf (string,
                              "window.%s .window-contents popover > contents { color: %s; background-color: alpha(%s, %s); }\n",
                              self->css_class, titlebar_fg, titlebar_bg, popover_alpha_str);
      g_string_append_printf (string,
                              "window.%s .window-contents popover > arrow { background-color: alpha(%s, %s); }\n",
                              self->css_class, titlebar_bg, popover_alpha_str);
      g_string_append_printf (string,
                              "window.%s .window-contents vte-terminal > revealer.size label { color: %s; background-color: %s; }\n",
                              self->css_class, titlebar_fg, revealer_bg);
      /* It would be super if we could make these match the color of the
       * actual tab contents rather than the active tab profile.
       */
      g_string_append_printf (string,
                              "window.%s .window-contents toolbarview.overview overlay.card { background-color: %s; color: %s; }\n",
                              self->css_class, bg, fg);
      g_string_append_printf (string,
                              "window.%s .window-contents toolbarview.overview tabthumbnail .icon-title-box { color: %s; }\n",
                              self->css_class, fg);
      g_string_append_printf (string,
                              "window.%s .window-contents toolbarview.overview { background-color: %s; color: %s; }\n",
                              self->css_class, titlebar_bg, titlebar_fg);
      g_string_append_printf (string,
                              "window.%s .window-contents revealer.raised.top-bar { background-color: %s; color: %s; }\n",
                              self->css_class, titlebar_bg, titlebar_fg);
      g_string_append_printf (string,
                              "window.%s .window-contents box.visual-bell headerbar { background-color: transparent; }\n"
                              "window.%s .window-contents box.visual-bell { animation: visual-bell-%s-%s 0.3s ease-out; }\n"
                              "@keyframes visual-bell-%s-%s { 50%% { background-color: %s; color: %s; } }\n",
                              self->css_class,
                              self->css_class, self->css_class, dark ? "dark" : "light",
                              self->css_class, dark ? "dark" : "light", bell_bg, bell_fg);
      g_string_append_printf (string,
                              "window.%s .window-contents banner > revealer > widget { background-color: %s; color: %s; }\n",
                              self->css_class, bell_bg, bell_fg);

      g_string_append_printf (string,
                              "window.%s taboverview.window-contents tabthumbnail .tab-close-button image { background-color: alpha(%s,.15); color: %s; }\n"
                              "window.%s taboverview.window-contents tabthumbnail .tab-close-button:hover image { background-color: alpha(%s,.25); }\n"
                              "window.%s taboverview.window-contents tabthumbnail .tab-close-button:active image { background-color: alpha(%s,.55); }\n",
                              self->css_class, fg, fg,
                              self->css_class, fg,
                              self->css_class, fg);

      visual_process_leader = ptyxis_settings_get_visual_process_leader (settings);

      g_string_append_printf (string,
                              "window.%s .window-contents > revealer windowhandle { color: %s; background-color: %s; }\n",
                              self->css_class, titlebar_fg, titlebar_bg);
      g_string_append_printf (string,
                              "window.%s:backdrop .window-contents revealer > windowhandle { color: mix(%s,%s,.025); background-color: mix(%s,%s,.99); }\n",
                              self->css_class, fg, bg, fg, bg);

      if (visual_process_leader)
        {
          g_string_append_printf (string,
                                  "window.%s.remote .window-contents headerbar { background-color: %s; color: %s; }\n"
                                  "window.%s.remote .window-contents toolbarview > revealer > windowhandle { background-color: %s; color: %s; }\n",
                                  self->css_class, rm_bg, rm_fg,
                                  self->css_class, rm_bg, rm_fg);
          g_string_append_printf (string,
                                  "window.%s.superuser .window-contents headerbar { background-color: %s; color: %s; }\n"
                                  "window.%s.superuser .window-contents toolbarview > revealer > windowhandle { background-color: %s; color: %s; }\n",
                                  self->css_class, su_bg, su_fg,
                                  self->css_class, su_bg, su_fg);
        }

      if (!ptyxis_palette_use_system_accent (self->palette))
        {
          accent_mix = face->indexed[4];
          accent_mix_str = gdk_rgba_to_string (&accent_mix);

          g_string_append_printf (string,
                                  "window.%s { --accent-fg-color: %s; --accent-bg-color: mix(%s,%s,.15); }\n",
                                  self->css_class,
                                  dark ? titlebar_fg : titlebar_bg,
                                  accent_mix_str,
                                  bg);
        }
    }

  gtk_css_provider_load_from_string (self->css_provider, string->str);
}

static gboolean
ptyxis_window_dressing_update_idle (gpointer user_data)
{
  PtyxisWindowDressing *self = PTYXIS_WINDOW_DRESSING (user_data);

  self->queued_update = 0;

  ptyxis_window_dressing_update (self);

  return G_SOURCE_REMOVE;
}

static void
ptyxis_window_dressing_queue_update (PtyxisWindowDressing *self)
{
  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));

  if (self->queued_update == 0)
    self->queued_update = g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                                           ptyxis_window_dressing_update_idle,
                                           self, NULL);
}

static void
ptyxis_window_dressing_set_window (PtyxisWindowDressing *self,
                                   PtyxisWindow         *window)
{
  g_assert (PTYXIS_IS_WINDOW_DRESSING (self));
  g_assert (PTYXIS_IS_WINDOW (window));

  g_weak_ref_set (&self->window_wr, window);

  gtk_widget_add_css_class (GTK_WIDGET (window), self->css_class);
}

static void
ptyxis_window_dressing_constructed (GObject *object)
{
  PtyxisWindowDressing *self = (PtyxisWindowDressing *)object;
  AdwStyleManager *style_manager = adw_style_manager_get_default ();
  PtyxisSettings *settings = ptyxis_application_get_settings (PTYXIS_APPLICATION_DEFAULT);
  g_autoptr(PtyxisWindow) window = NULL;

  G_OBJECT_CLASS (ptyxis_window_dressing_parent_class)->constructed (object);

  window = ptyxis_window_dressing_dup_window (self);
  if (window != NULL)
    ptyxis_window_dressing_create_session_context_chip (self, window);

  g_signal_connect_object (settings,
                           "notify::visual-process-leader",
                           G_CALLBACK (ptyxis_window_dressing_queue_update),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                              GTK_STYLE_PROVIDER (self->css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER + 1);

  g_signal_connect_object (style_manager,
                           "notify::dark",
                           G_CALLBACK (ptyxis_window_dressing_queue_update),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (style_manager,
                           "notify::color-scheme",
                           G_CALLBACK (ptyxis_window_dressing_queue_update),
                           self,
                           G_CONNECT_SWAPPED);

  ptyxis_window_dressing_queue_update (self);
}

static void
ptyxis_window_dressing_dispose (GObject *object)
{
  PtyxisWindowDressing *self = (PtyxisWindowDressing *)object;

  ptyxis_window_dressing_set_palette (self, NULL);

  g_signal_group_set_target (self->session_context_tab_signals, NULL);
  g_clear_object (&self->session_context_tab_signals);
  self->session_context_chip = NULL;
  self->session_context_icon = NULL;
  self->session_context_label = NULL;

  if (self->css_provider != NULL) {
    gtk_style_context_remove_provider_for_display (gdk_display_get_default (),
                                                   GTK_STYLE_PROVIDER (self->css_provider));
    g_clear_object (&self->css_provider);
  }

  g_clear_object (&self->palette);
  g_weak_ref_set (&self->window_wr, NULL);

  g_clear_handle_id (&self->queued_update, g_source_remove);

  G_OBJECT_CLASS (ptyxis_window_dressing_parent_class)->dispose (object);
}

static void
ptyxis_window_dressing_finalize (GObject *object)
{
  PtyxisWindowDressing *self = (PtyxisWindowDressing *)object;

  g_weak_ref_clear (&self->window_wr);
  g_clear_pointer (&self->css_class, g_free);

  g_assert (self->palette == NULL);
  g_assert (self->queued_update == 0);
  g_assert (self->css_provider == NULL);
  g_assert (self->session_context_tab_signals == NULL);

  G_OBJECT_CLASS (ptyxis_window_dressing_parent_class)->finalize (object);
}

static void
ptyxis_window_dressing_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PtyxisWindowDressing *self = PTYXIS_WINDOW_DRESSING (object);

  switch (prop_id)
    {
    case PROP_OPACITY:
      g_value_set_double (value, ptyxis_window_dressing_get_opacity (self));
      break;

    case PROP_PALETTE:
      g_value_set_object (value, ptyxis_window_dressing_get_palette (self));
      break;

    case PROP_WINDOW:
      g_value_take_object (value, ptyxis_window_dressing_dup_window (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ptyxis_window_dressing_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  PtyxisWindowDressing *self = PTYXIS_WINDOW_DRESSING (object);

  switch (prop_id)
    {
    case PROP_OPACITY:
      ptyxis_window_dressing_set_opacity (self, g_value_get_double (value));
      break;

    case PROP_PALETTE:
      ptyxis_window_dressing_set_palette (self, g_value_get_object (value));
      break;

    case PROP_WINDOW:
      ptyxis_window_dressing_set_window (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ptyxis_window_dressing_class_init (PtyxisWindowDressingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = ptyxis_window_dressing_constructed;
  object_class->dispose = ptyxis_window_dressing_dispose;
  object_class->finalize = ptyxis_window_dressing_finalize;
  object_class->get_property = ptyxis_window_dressing_get_property;
  object_class->set_property = ptyxis_window_dressing_set_property;

  properties[PROP_OPACITY] =
    g_param_spec_double ("opacity", NULL, NULL,
                         0, 1, 1,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_PALETTE] =
    g_param_spec_object ("palette", NULL, NULL,
                         PTYXIS_TYPE_PALETTE,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_WINDOW] =
    g_param_spec_object ("window", NULL, NULL,
                         PTYXIS_TYPE_WINDOW,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
ptyxis_window_dressing_init (PtyxisWindowDressing *self)
{
  self->css_provider = gtk_css_provider_new ();
  self->css_class = g_strdup_printf ("window-dressing-%u", ++last_sequence);
  self->opacity = 1.0;
  self->session_context_tab_signals = g_signal_group_new (PTYXIS_TYPE_TAB);

  g_signal_group_connect_object (self->session_context_tab_signals,
                                 "notify::process-leader-kind",
                                 G_CALLBACK (ptyxis_window_dressing_session_context_tab_notify_cb),
                                 self,
                                 G_CONNECT_SWAPPED);

  g_weak_ref_init (&self->window_wr, NULL);
}

PtyxisWindow *
ptyxis_window_dressing_dup_window (PtyxisWindowDressing *self)
{
  g_return_val_if_fail (PTYXIS_IS_WINDOW_DRESSING (self), NULL);

  return PTYXIS_WINDOW (g_weak_ref_get (&self->window_wr));
}

PtyxisWindowDressing *
ptyxis_window_dressing_new (PtyxisWindow *window)
{
  g_return_val_if_fail (PTYXIS_IS_WINDOW (window), NULL);

  return g_object_new (PTYXIS_TYPE_WINDOW_DRESSING,
                       "window", window,
                       NULL);
}

PtyxisPalette *
ptyxis_window_dressing_get_palette (PtyxisWindowDressing *self)
{
  g_return_val_if_fail (PTYXIS_IS_WINDOW_DRESSING (self), NULL);

  return self->palette;
}

void
ptyxis_window_dressing_set_palette (PtyxisWindowDressing *self,
                                    PtyxisPalette        *palette)
{
  g_return_if_fail (PTYXIS_IS_WINDOW_DRESSING (self));
  g_return_if_fail (!palette || PTYXIS_IS_PALETTE (palette));

  if (g_set_object (&self->palette, palette))
    {
      ptyxis_window_dressing_queue_update (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PALETTE]);
    }
}

double
ptyxis_window_dressing_get_opacity (PtyxisWindowDressing *self)
{
  g_return_val_if_fail (PTYXIS_IS_WINDOW_DRESSING (self), 1.);

  return self->opacity;
}

void
ptyxis_window_dressing_set_opacity (PtyxisWindowDressing *self,
                                    double                opacity)
{
  g_return_if_fail (PTYXIS_IS_WINDOW_DRESSING (self));

  if (opacity != self->opacity)
    {
      self->opacity = opacity;
      ptyxis_window_dressing_queue_update (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_OPACITY]);
    }
}
