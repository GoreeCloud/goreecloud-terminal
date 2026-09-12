project(
  'goreecloud-terminal-native',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

cc = meson.get_compiler('c')
native_executable_c_args = []

# glib-compile-resources may emit a generated C string larger than the ISO C
# minimum guaranteed 4095-character literal length when embedding the Glaze UI
# stylesheet. Keep every other warning fatal and demote only this generated-
# resource portability warning for the native application target.
if cc.has_argument('-Wno-error=overlength-strings')
  native_executable_c_args += '-Wno-error=overlength-strings'
endif

glib = dependency('glib-2.0', version: '>=2.76', include_type: 'system')
gio = dependency('gio-2.0', version: '>=2.76', include_type: 'system')
gtk = dependency('gtk4', version: '>=4.14', include_type: 'system')
vte = dependency('vte-2.91-gtk4', version: '>=0.76', include_type: 'system')
gnome = import('gnome')

native_resources = gnome.compile_resources(
  'goreecloud-terminal-native-resources',
  'data/goreecloud-terminal-native.gresource.xml',
  source_dir: 'data',
  c_name: 'goreecloud_terminal_native',
)

executable(
  'goreecloud-terminal-native',
  [
    'src/main.c',
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
    'src/remote-session.c',
    'src/remote-openssh.c',
    'src/remote-target-store.c',
    'src/remote-privacy-adapter.c',
    native_resources,
  ],
  include_directories: include_directories('include'),
  dependencies: [glib, gio, gtk, vte],
  c_args: native_executable_c_args,
  install: false,
)

session_lifecycle_test = executable(
  'test-native-session-lifecycle',
  [
    'src/test-session-lifecycle.c',
    'src/session-lifecycle.c',
  ],
  dependencies: [glib],
)

glaze_contract_test = executable(
  'test-native-glaze-contract',
  [
    'src/test-glaze-contract.c',
    'src/glaze-contract.c',
  ],
  dependencies: [glib],
)

theme_engine_test = executable(
  'test-native-theme-engine',
  [
    'src/test-theme-engine.c',
    'src/theme-engine.c',
  ],
  dependencies: [glib],
)

preferences_test = executable(
  'test-native-terminal-preferences',
  [
    'src/test-terminal-preferences.c',
    'src/terminal-preferences.c',
  ],
  dependencies: [glib],
)

paste_guard_test = executable(
  'test-native-paste-guard',
  [
    'src/test-paste-guard.c',
    'src/paste-guard.c',
    'src/terminal-preferences.c',
  ],
  dependencies: [glib],
)

session_profile_test = executable(
  'test-native-session-profile',
  [
    'src/test-session-profile.c',
    'src/session-profile.c',
    'src/terminal-preferences.c',
  ],
  dependencies: [glib],
)

workspace_store_test = executable(
  'test-native-workspace-store',
  [
    'src/test-workspace-store.c',
    'src/workspace-store.c',
  ],
  dependencies: [glib],
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
  dependencies: [glib],
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
  dependencies: [glib],
)

remote_session_test = executable(
  'test-native-remote-session',
  [
    'src/test-remote-session.c',
    'src/remote-session.c',
    'src/remote-openssh.c',
  ],
  dependencies: [glib],
)

remote_target_store_test = executable(
  'test-native-remote-target-store',
  [
    'src/test-remote-target-store.c',
    'src/remote-target-store.c',
    'src/remote-session.c',
  ],
  dependencies: [glib],
)

remote_privacy_adapter_test = executable(
  'test-native-remote-privacy-adapter',
  [
    'src/test-remote-privacy-adapter.c',
    'src/remote-privacy-adapter.c',
    'src/remote-session.c',
  ],
  dependencies: [glib],
)

test('native-session-lifecycle', session_lifecycle_test)
test('native-glaze-contract', glaze_contract_test)
test('native-theme-engine', theme_engine_test)
test('native-terminal-preferences', preferences_test)
test('native-paste-guard', paste_guard_test)
test('native-session-profile', session_profile_test)
test('native-workspace-store', workspace_store_test)
test('native-profile-runtime', profile_runtime_test)
test('native-workspace-runtime', workspace_runtime_test)
test('native-remote-session', remote_session_test)
test('native-remote-target-store', remote_target_store_test)
test('native-remote-privacy-adapter', remote_privacy_adapter_test)
