/*
 * goreecloud-session-context.h
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ptyxis-tab.h"

G_BEGIN_DECLS

typedef enum
{
  GOREECLOUD_SESSION_CONTEXT_SEVERITY_NONE,
  GOREECLOUD_SESSION_CONTEXT_SEVERITY_INFO,
  GOREECLOUD_SESSION_CONTEXT_SEVERITY_CAUTION,
  GOREECLOUD_SESSION_CONTEXT_SEVERITY_ELEVATED,
} GoreeCloudSessionContextSeverity;

typedef struct
{
  const char                         *label;
  const char                         *accessible_description;
  const char                         *icon_name;
  const char                         *css_class;
  GoreeCloudSessionContextSeverity   severity;
  gboolean                           show_indicator;
} GoreeCloudSessionContext;

const GoreeCloudSessionContext *goreecloud_session_context_for_leader_kind (PtyxisProcessLeaderKind kind);

G_END_DECLS
