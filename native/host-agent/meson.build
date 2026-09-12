project(
  'goreecloud-terminal-host-agent',
  'c',
  version: '0.1.0-dev',
  default_options: ['c_std=c17', 'warning_level=3', 'werror=true'],
)

cc = meson.get_compiler('c')
util = cc.find_library('util', required: true)

host_agent_name = 'goreecloud-terminal-host-agent'
host_agent_path = join_paths(
  get_option('prefix'),
  get_option('libexecdir'),
  host_agent_name,
)

executable(
  host_agent_name,
  'main.c',
  include_directories: include_directories('../include'),
  dependencies: [util],
  install: true,
  install_dir: get_option('libexecdir'),
)

service_configuration = configuration_data()
service_configuration.set('HOST_AGENT_PATH', host_agent_path)

configure_file(
  input: 'goreecloud-terminal-host-agent.service.in',
  output: 'goreecloud-terminal-host-agent.service',
  configuration: service_configuration,
  install: true,
  install_dir: join_paths(get_option('prefix'), 'lib/systemd/user'),
)
