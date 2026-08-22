man_config = configuration_data()
man_config.set('app_id', app_id)

compat_man_page = configure_file(
          input: 'ptyxis.1.in',
         output: 'ptyxis.1',
  configuration: man_config,
)

product_man_page = configure_file(
          input: 'goreecloud-terminal.1.in',
         output: 'goreecloud-terminal.1',
  configuration: man_config,
)

install_man(compat_man_page, product_man_page)
