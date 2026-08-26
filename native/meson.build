project(
  'goreecloud-terminal-native',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

gtk = dependency('gtk4', version: '>=4.14')
vte = dependency('vte-2.91-gtk4', version: '>=0.76')

executable(
  'goreecloud-terminal-native',
  'src/main.c',
  dependencies: [gtk, vte],
  install: false,
)
