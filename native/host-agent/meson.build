project(
  'goreecloud-terminal-host-agent',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

cc = meson.get_compiler('c')
util = cc.find_library('util', required: true)

executable(
  'goreecloud-terminal-host-agent',
  'main.c',
  include_directories: include_directories('../include'),
  dependencies: [util],
  install: false,
)
