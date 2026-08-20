/*
 * test-goreecloud-session-context.c
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <glib.h>

#include "goreecloud-session-context.h"

static void
assert_context (PtyxisProcessLeaderKind             kind,
                const char                         *label,
                const char                         *description,
                const char                         *icon_name,
                const char                         *css_class,
                GoreeCloudSessionContextSeverity   severity,
                gboolean                           show_indicator)
{
  const GoreeCloudSessionContext *context;

  context = goreecloud_session_context_for_leader_kind (kind);

  g_assert_nonnull (context);
  g_assert_cmpstr (context->label, ==, label);
  g_assert_cmpstr (context->accessible_description, ==, description);
  g_assert_cmpstr (context->icon_name, ==, icon_name);
  g_assert_cmpstr (context->css_class, ==, css_class);
  g_assert_cmpint (context->severity, ==, severity);
  g_assert_cmpint (context->show_indicator, ==, show_indicator);
}

static void
test_local_context (void)
{
  assert_context (PTYXIS_PROCESS_LEADER_KIND_UNKNOWN,
                  "Local",
                  "Local terminal session",
                  NULL,
                  "session-context-local",
                  GOREECLOUD_SESSION_CONTEXT_SEVERITY_NONE,
                  FALSE);
}

static void
test_remote_context (void)
{
  assert_context (PTYXIS_PROCESS_LEADER_KIND_REMOTE,
                  "Remote",
                  "Remote terminal session",
                  "process-remote-symbolic",
                  "session-context-remote",
                  GOREECLOUD_SESSION_CONTEXT_SEVERITY_INFO,
                  TRUE);
}

static void
test_container_context (void)
{
  assert_context (PTYXIS_PROCESS_LEADER_KIND_CONTAINER,
                  "Container",
                  "Containerized terminal session",
                  "container-generic-symbolic",
                  "session-context-container",
                  GOREECLOUD_SESSION_CONTEXT_SEVERITY_CAUTION,
                  TRUE);
}

static void
test_elevated_context (void)
{
  assert_context (PTYXIS_PROCESS_LEADER_KIND_SUPERUSER,
                  "Elevated",
                  "Elevated superuser terminal session",
                  "process-superuser-symbolic",
                  "session-context-elevated",
                  GOREECLOUD_SESSION_CONTEXT_SEVERITY_ELEVATED,
                  TRUE);
}

static void
test_unknown_values_fail_closed_to_local (void)
{
  const GoreeCloudSessionContext *context;

  context = goreecloud_session_context_for_leader_kind ((PtyxisProcessLeaderKind)99);

  g_assert_nonnull (context);
  g_assert_cmpstr (context->label, ==, "Local");
  g_assert_false (context->show_indicator);
  g_assert_cmpint (context->severity, ==, GOREECLOUD_SESSION_CONTEXT_SEVERITY_NONE);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/goreecloud/session-context/local", test_local_context);
  g_test_add_func ("/goreecloud/session-context/remote", test_remote_context);
  g_test_add_func ("/goreecloud/session-context/container", test_container_context);
  g_test_add_func ("/goreecloud/session-context/elevated", test_elevated_context);
  g_test_add_func ("/goreecloud/session-context/unknown-fails-closed", test_unknown_values_fail_closed_to_local);

  return g_test_run ();
}
