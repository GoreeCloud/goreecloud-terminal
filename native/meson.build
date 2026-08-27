project(
  'goreecloud-terminal-native',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

glib = dependency('glib-2.0', version: '>=2.76', include_type: 'system')
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
    'src/glaze-contract.c',
    native_resources,
  ],
  dependencies: [glib, gtk, vte],
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

test('native-session-lifecycle', session_lifecycle_test)
test('native-glaze-contract', glaze_contract_test)
