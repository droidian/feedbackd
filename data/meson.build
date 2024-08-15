# DBus server interface
prefix = 'org.sigxcpu.'
namespace = 'LfbGdbus'

generated_dbus_sources = []

dbus_interfaces = [ 'org.sigxcpu.Feedback.xml' ]

generated_dbus_sources += gnome.gdbus_codegen('lfb-gdbus',
      sources          : dbus_interfaces,
      object_manager   : false,
      docbook          : 'libfeedback',
      interface_prefix : prefix,
      install_header   : true,
      install_dir      : join_paths(get_option('includedir'), libname),
      namespace        : namespace)

dbus_header_dir = meson.current_build_dir()
dbus_inc = include_directories('.')
install_data(dbus_interfaces, install_dir: ifacedir)

schemas = ['org.sigxcpu.feedbackd.gschema.xml']
compiled = gnome.compile_schemas(
  build_by_default: true
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file', compile_schemas,
       args: ['--strict', '--dry-run', meson.current_source_dir()])
endif

install_data(
  schemas,
  install_dir: 'share/glib-2.0/schemas'
)


if get_option('daemon')

service_data = configuration_data()
service_data.set('libexecdir', libexecdir)
service_file = 'org.sigxcpu.Feedback.service'

configure_file(
  input : service_file + '.in',
  output : service_file,
  configuration : service_data,
  install : true,
  install_dir: servicedir,
)

theme_json = ['default.json']
install_data(
  theme_json,
  install_dir: feedbackd_theme_dir,
)

if json_glib_validate.found()
  jsons = []
  foreach theme : theme_json
    jsons += (join_paths(meson.current_source_dir(), theme))
  endforeach
  custom_target('validate-json',
		build_by_default: true,
		input: theme_json,
		output: 'json_validate_stamp',
		command: [json_glib_validate.full_path()] + jsons)
endif

if get_option('udev')
  install_data('90-feedbackd.rules', install_dir: udevdir)
endif

endif # daemon
