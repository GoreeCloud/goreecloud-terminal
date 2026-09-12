#include <glib.h>

#include "product-identity.h"

static void
test_canonical_name_and_launcher(void)
{
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_PRODUCT_NAME,
        ==,
        "GoreeCloud Terminal");
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_CANONICAL_LAUNCHER,
        ==,
        "goreecloud-terminal");
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_RESOURCE_PREFIX,
        ==,
        "/com/goreecloud/Terminal");
}

static void
test_selected_application_identity(void)
{
#if GOREECLOUD_TERMINAL_DEVELOPMENT_BUILD
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_APPLICATION_ID,
        ==,
        "com.goreecloud.Terminal.Devel");
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_ICON_NAME,
        ==,
        "com.goreecloud.Terminal.Devel");
#else
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_APPLICATION_ID,
        ==,
        "com.goreecloud.Terminal");
    g_assert_cmpstr(
        GOREECLOUD_TERMINAL_ICON_NAME,
        ==,
        "com.goreecloud.Terminal");
#endif

    g_assert_null(
        g_strstr_len(
            GOREECLOUD_TERMINAL_APPLICATION_ID,
            -1,
            ".Native"));
}

static void
test_product_version(void)
{
    g_assert_nonnull(GOREECLOUD_TERMINAL_VERSION);
    g_assert_cmpstr(GOREECLOUD_TERMINAL_VERSION, ==, "0.1.0-dev");
    g_assert_null(g_strstr_len(GOREECLOUD_TERMINAL_VERSION, -1, " "));
    g_assert_null(g_strstr_len(GOREECLOUD_TERMINAL_VERSION, -1, "\n"));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(
        "/product-identity/name-launcher",
        test_canonical_name_and_launcher);
    g_test_add_func(
        "/product-identity/application-id",
        test_selected_application_identity);
    g_test_add_func(
        "/product-identity/version",
        test_product_version);
    return g_test_run();
}
