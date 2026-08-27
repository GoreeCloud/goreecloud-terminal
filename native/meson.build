project(
  'goreecloud-terminal-native',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

glib = dependency('glib-2.0', version: '>=2.76')
gtk = dependency('gtk4', version: '>=4.14')
vte = dependency('vte-2.91-gtk4', version: '>=0.76')

executable(
  'goreecloud-terminal-native',
  [
    'src/main.c',
    'src/session-lifecycle.c',
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

test('native-session-lifecycle', session_lifecycle_test)
