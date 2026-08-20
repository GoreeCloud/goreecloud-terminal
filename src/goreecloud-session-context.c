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

#if DEVELOPMENT_BUILD
static const GoreeCloudSessionContext *
goreecloud_session_context_from_preview (void)
{
  const char *preview = g_getenv ("GORECLOUD_SESSION_CONTEXT_PREVIEW");

  if (preview == NULL || *preview == '\0')
    return NULL;

  if (g_str_equal (preview, "local"))
    return &local_context;
  if (g_str_equal (preview, "remote"))
    return &remote_context;
  if (g_str_equal (preview, "container"))
    return &container_context;
  if (g_str_equal (preview, "elevated"))
    return &elevated_context;

  g_warning ("Ignoring unsupported GORECLOUD_SESSION_CONTEXT_PREVIEW value '%s'", preview);
  return NULL;
}
#endif

const GoreeCloudSessionContext *
goreecloud_session_context_for_leader_kind (PtyxisProcessLeaderKind kind)
{
#if DEVELOPMENT_BUILD
  const GoreeCloudSessionContext *preview = goreecloud_session_context_from_preview ();

  if (preview != NULL)
    return preview;
#endif

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
