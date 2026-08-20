/*
 * goreecloud-session-context.c
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "goreecloud-session-context.h"

static const GoreeCloudSessionContext local_context = {
  .label = "Local",
  .accessible_description = "Local terminal session",
  .icon_name = NULL,
  .css_class = "session-context-local",
  .severity = GOREECLOUD_SESSION_CONTEXT_SEVERITY_NONE,
  .show_indicator = FALSE,
};

static const GoreeCloudSessionContext remote_context = {
  .label = "Remote",
  .accessible_description = "Remote terminal session",
  .icon_name = "process-remote-symbolic",
  .css_class = "session-context-remote",
  .severity = GOREECLOUD_SESSION_CONTEXT_SEVERITY_INFO,
  .show_indicator = TRUE,
};

static const GoreeCloudSessionContext container_context = {
  .label = "Container",
  .accessible_description = "Containerized terminal session",
  .icon_name = "container-generic-symbolic",
  .css_class = "session-context-container",
  .severity = GOREECLOUD_SESSION_CONTEXT_SEVERITY_CAUTION,
  .show_indicator = TRUE,
};

static const GoreeCloudSessionContext elevated_context = {
  .label = "Elevated",
  .accessible_description = "Elevated superuser terminal session",
  .icon_name = "process-superuser-symbolic",
  .css_class = "session-context-elevated",
  .severity = GOREECLOUD_SESSION_CONTEXT_SEVERITY_ELEVATED,
  .show_indicator = TRUE,
};

const GoreeCloudSessionContext *
goreecloud_session_context_for_leader_kind (PtyxisProcessLeaderKind kind)
{
  switch (kind)
    {
    case PTYXIS_PROCESS_LEADER_KIND_REMOTE:
      return &remote_context;

    case PTYXIS_PROCESS_LEADER_KIND_CONTAINER:
      return &container_context;

    case PTYXIS_PROCESS_LEADER_KIND_SUPERUSER:
      return &elevated_context;

    case PTYXIS_PROCESS_LEADER_KIND_UNKNOWN:
    default:
      return &local_context;
    }
}
