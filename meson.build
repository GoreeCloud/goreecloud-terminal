project('ptyxis', 'c',
          version: '50.2',
    meson_version: '>= 1.0.0',
  default_options: [ 'warning_level=2', 'werror=false', 'c_std=gnu11', ],
)

cc = meson.get_compiler('c')

i18n = import('i18n')
gnome = import('gnome')

app_schema_id = get_option('gschema-id')
app_schema_path = get_option('gschema-path')

app_is_generic = get_option('generic') != 'ptyxis'
app_is_builder = get_option('generic') == 'builder'
app_id = get_option('app-id')

if get_option('development')
  if app_id == ''
    app_id = 'com.goreecloud.Terminal.Devel'
  endif
  if app_schema_path == ''
    app_schema_path = '/com/goreecloud/Terminal/Devel/'
  endif
else
  if app_id == ''
    app_id = 'com.goreecloud.Terminal'
  endif
  if app_schema_path == ''
    app_schema_path = '/com/goreecloud/Terminal/'
  endif
endif

if app_schema_id == ''
  if get_option('development')
    app_schema_id = 'com.goreecloud.Terminal.Devel'
  else
    app_schema_id = 'com.goreecloud.Terminal'
  endif
endif

app_schema_profile_id = '@0@.Profile'.format(app_schema_id)
app_schema_shortcuts_id = '@0@.Shortcuts'.format(app_schema_id)
app_schema_shortcuts_path = '@0@Shortcuts/'.format(app_schema_path)

glib_req_version = '2.80'
gtk_req_version = '4.14'
adwaita_req_version = '1.8'
json_glib_req_version = '1.6'
vte_req_version = '0.79'

glib_req = '>= @0@'.format(glib_req_version)
gtk_req = '>= @0@'.format(gtk_req_version)
adwaita_req = '>= @0@'.format(adwaita_req_version)
json_glib_req = '>= @0@'.format(json_glib_req_version)
vte_req = '>= @0@'.format(vte_req_version)

# Various optimizations for non-debug builds including disabling of
# cast checks, asserts, and additional link options.
release_link_args = []
test_link_args = [
  '-Wl,-z,defs',   # Detect and reject underlinking
  '-Wl,-z,now',    # Disable lazy binding
  '-Wl,-z,relro',  # Read-only segments after relocation
]
if not get_option('buildtype').startswith('debug')
  add_global_arguments([
    '-DG_DISABLE_ASSERT',
    '-DG_DISABLE_CAST_CHECKS',
  ], language: 'c')
  test_link_args += ['-Wl,-Bsymbolic', '-fno-plt']
endif

config_h = configuration_data()
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('APP_SCHEMA_ID', app_schema_id)
config_h.set_quoted('APP_SCHEMA_PATH', app_schema_path)
config_h.set_quoted('APP_SCHEMA_PROFILE_ID', app_schema_profile_id)
config_h.set_quoted('APP_SCHEMA_SHORTCUTS_ID', app_schema_shortcuts_id)
config_h.set_quoted('APP_SCHEMA_SHORTCUTS_PATH', app_schema_shortcuts_path)
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'ptyxis')
config_h.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
# LIBEXECDIR must be expanded in config.h
config_h.set_quoted('LIBEXECDIR', get_option('prefix') / get_option('libexecdir'))
config_h.set_quoted('PACKAGE_WEBSITE', 'https://github.com/GoreeCloud/goreecloud-terminal')
config_h.set_quoted('PACKAGE_ICON_NAME', app_id)
config_h.set_quoted('PACKAGE_NAME', 'GoreeCloud Terminal')
config_h.set10('DEVELOPMENT_BUILD', get_option('development'))
config_h.set10('APP_IS_GENERIC', app_is_generic)
config_h.set10('APP_IS_BUILDER', app_is_builder)

summary('Application ID', app_id)
summary('GSchema ID Prefix', app_schema_id)
summary('GSchema Path Prefix', app_schema_path)
summary('Development Banner', get_option('development'))
summary('Generic', app_is_generic)

check_functions = [
  'fchmod',
  'setlocale',
  'getuid',
  'geteuid',
  'getpwuid',
  'setenv',
  'unsetenv',
  'setrlimit',
  'getrlimit',
  'prctl',
  'setsid',
  'setpgid',
  'setpgrp',
  'tcsetpgrp',
  'statvfs',
]

foreach check_function: check_functions
  config_h.set10('HAVE_' + check_function.to_upper(), cc.has_function(check_function))
endforeach

configure_file(
         output: 'config.h',
  configuration: config_h,
)

subdir('src')
subdir('agent')
subdir('data')
subdir('po')
subdir('man')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
