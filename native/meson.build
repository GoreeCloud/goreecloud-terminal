project(
  'goreecloud-terminal-native',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

cc = meson.get_compiler('c')
python3 = find_program('python3')
gnome = import('gnome')
native_executable_c_args = []

# Host-native migration maintenance intentionally has a lower dependency floor
# than the GTK/VTE application. This allows controlled older workstation
# distributions to build migration/rollback maintenance without weakening the
# GUI runtime requirements or bundling a private desktop stack.
glib_maintenance = dependency('glib-2.0', version: '>=2.72', include_type: 'system')
gio_maintenance = dependency('gio-2.0', version: '>=2.72', include_type: 'system')

build_application = get_option('build_application')
product_identity = get_option('product_identity')
install_migration_tool = get_option('install_migration_tool')
product_name = 'GoreeCloud Terminal'
product_version = meson.project_version()
resource_prefix = '/com/goreecloud/Terminal'

if product_identity == 'production'
  application_id = 'com.goreecloud.Terminal'
  development_build = 0
  icon_source = 'data/icons/goreecloud-terminal.svg'
else
  application_id = 'com.goreecloud.Terminal.Devel'
  development_build = 1
  icon_source = 'data/icons/goreecloud-terminal-devel.svg'
endif

migration_schema_source = join_paths(
  meson.current_source_dir(),
  'data',
  'com.goreecloud.Terminal.Migration.gschema.xml',
)

if install_migration_tool
  install_data(
    'data/com.goreecloud.Terminal.Migration.gschema.xml',
    install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'),
  )
  gnome.post_install(glib_compile_schemas: true)

  executable(
    'goreecloud-terminal-migrate',
    [
      'src/migration-tool.c',
      'src/legacy-migration.c',
      'src/terminal-preferences.c',
      'src/session-profile.c',
      'src/workspace-store.c',
    ],
    include_directories: include_directories('src'),
    dependencies: [glib_maintenance, gio_maintenance],
    install: true,
    install_dir: get_option('bindir'),
  )
endif

preferences_test = executable(
  'test-native-terminal-preferences',
  [
    'src/test-terminal-preferences.c',
    'src/terminal-preferences.c',
  ],
  dependencies: [glib_maintenance],
)

session_profile_test = executable(
  'test-native-session-profile',
  [
    'src/test-session-profile.c',
    'src/session-profile.c',
    'src/terminal-preferences.c',
  ],
  dependencies: [glib_maintenance],
)

workspace_store_test = executable(
  'test-native-workspace-store',
  [
    'src/test-workspace-store.c',
    'src/workspace-store.c',
  ],
  dependencies: [glib_maintenance],
)

legacy_migration_test = executable(
  'test-native-legacy-migration',
  [
    'src/test-legacy-migration.c',
    'src/legacy-migration.c',
    'src/terminal-preferences.c',
    'src/session-profile.c',
    'src/workspace-store.c',
  ],
  include_directories: include_directories('src'),
  dependencies: [glib_maintenance, gio_maintenance],
  c_args: [
    '-DMIGRATION_SCHEMA_SOURCE="@0@"'.format(migration_schema_source),
  ],
)

test('native-terminal-preferences', preferences_test)
test('native-session-profile', session_profile_test)
test('native-workspace-store', workspace_store_test)
test('native-legacy-migration', legacy_migration_test)

if build_application
  # The GUI runtime keeps the stronger application dependency floor. The
  # maintenance-only build path above must never be interpreted as lowering
  # these requirements for the actual Terminal application.
  glib_application = dependency('glib-2.0', version: '>=2.76', include_type: 'system')
  gio_application = dependency('gio-2.0', version: '>=2.76', include_type: 'system')
  gtk = dependency('gtk4', version: '>=4.14', include_type: 'system')
  vte = dependency('vte-2.91-gtk4', version: '>=0.76', include_type: 'system')

  # glib-compile-resources may emit a generated C string larger than the ISO C
  # minimum guaranteed 4095-character literal length when embedding the Glaze
  # UI stylesheet. Keep every other warning fatal and demote only this
  # generated-resource portability warning for the native application target.
  if cc.has_argument('-Wno-error=overlength-strings')
    native_executable_c_args += '-Wno-error=overlength-strings'
  endif

  identity_config = configuration_data()
  identity_config.set('PRODUCT_NAME', product_name)
  identity_config.set('PRODUCT_VERSION', product_version)
  identity_config.set('APPLICATION_ID', application_id)
  identity_config.set('ICON_NAME', application_id)
  identity_config.set('DEVELOPMENT_BUILD', development_build)

  configure_file(
    input: 'src/product-identity.h.in',
    output: 'product-identity.h',
    configuration: identity_config,
  )

  configure_file(
    input: 'data/goreecloud-terminal.desktop.in',
    output: application_id + '.desktop',
    configuration: identity_config,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications'),
  )

  configure_file(
    input: 'data/goreecloud-terminal.metainfo.xml.in',
    output: application_id + '.metainfo.xml',
    configuration: identity_config,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'metainfo'),
  )

  configure_file(
    input: icon_source,
    output: application_id + '.svg',
    copy: true,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', 'scalable', 'apps'),
  )

  render_product_main = files('tools/render-product-main.py')
  identity_main = custom_target(
    'goreecloud-terminal-product-main',
    input: 'src/main.c',
    output: 'main-product-identity.c',
    command: [
      python3,
      render_product_main,
      '@INPUT@',
      '@OUTPUT@',
      application_id,
      resource_prefix,
      product_version,
    ],
    build_by_default: true,
  )

  native_resources = gnome.compile_resources(
    'goreecloud-terminal-native-resources',
    'data/goreecloud-terminal-native.gresource.xml',
    source_dir: 'data',
    c_name: 'goreecloud_terminal_native',
  )

  executable(
    'goreecloud-terminal',
    [
      identity_main,
      'src/session-lifecycle.c',
      'src/host-session-client.c',
      'src/glaze-contract.c',
      'src/theme-engine.c',
      'src/terminal-preferences.c',
      'src/paste-guard.c',
      'src/session-profile.c',
      'src/workspace-store.c',
      'src/profile-runtime.c',
      'src/workspace-runtime.c',
      'src/workspace-view.c',
      native_resources,
    ],
    include_directories: [
      include_directories('.'),
      include_directories('include'),
      include_directories('src'),
    ],
    dependencies: [glib_application, gio_application, gtk, vte],
    c_args: native_executable_c_args,
    install: true,
    install_dir: get_option('bindir'),
  )

  session_lifecycle_test = executable(
    'test-native-session-lifecycle',
    [
      'src/test-session-lifecycle.c',
      'src/session-lifecycle.c',
    ],
    dependencies: [glib_application],
  )

  glaze_contract_test = executable(
    'test-native-glaze-contract',
    [
      'src/test-glaze-contract.c',
      'src/glaze-contract.c',
    ],
    dependencies: [glib_application],
  )

  theme_engine_test = executable(
    'test-native-theme-engine',
    [
      'src/test-theme-engine.c',
      'src/theme-engine.c',
    ],
    dependencies: [glib_application],
  )

  paste_guard_test = executable(
    'test-native-paste-guard',
    [
      'src/test-paste-guard.c',
      'src/paste-guard.c',
      'src/terminal-preferences.c',
    ],
    dependencies: [glib_application],
  )

  profile_runtime_test = executable(
    'test-native-profile-runtime',
    [
      'src/test-profile-runtime.c',
      'src/profile-runtime.c',
      'src/session-profile.c',
      'src/workspace-store.c',
      'src/terminal-preferences.c',
    ],
    include_directories: include_directories('include'),
    dependencies: [glib_application],
  )

  workspace_runtime_test = executable(
    'test-native-workspace-runtime',
    [
      'src/test-workspace-runtime.c',
      'src/workspace-runtime.c',
      'src/profile-runtime.c',
      'src/session-profile.c',
      'src/workspace-store.c',
      'src/terminal-preferences.c',
    ],
    include_directories: include_directories('include'),
    dependencies: [glib_application],
  )

  product_identity_test = executable(
    'test-native-product-identity',
    'src/test-product-identity.c',
    include_directories: include_directories('.'),
    dependencies: [glib_application],
  )

  test('native-session-lifecycle', session_lifecycle_test)
  test('native-glaze-contract', glaze_contract_test)
  test('native-theme-engine', theme_engine_test)
  test('native-paste-guard', paste_guard_test)
  test('native-profile-runtime', profile_runtime_test)
  test('native-workspace-runtime', workspace_runtime_test)
  test('native-product-identity', product_identity_test)
endif

summary(
  {
    'Product name': product_name,
    'Product version': product_version,
    'Product identity': product_identity,
    'Application ID': application_id,
    'Build GTK/VTE application': build_application,
    'Maintenance GLib/GIO floor': '>=2.72',
    'GUI GLib/GIO floor': '>=2.76 when build_application=true',
    'Canonical launcher': 'goreecloud-terminal',
    'Install host migration tool': install_migration_tool,
    'Installable staged layout': true,
  },
  section: 'GoreeCloud Terminal native identity',
)
