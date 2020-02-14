# Feedback daemon
if get_option('daemon')

fbd_enum_headers = files([
  'fbd-event.h',
  'fbd-feedback-vibra.h',
])
fbd_enum_sources = gnome.mkenums_simple('fbd-enums',
  sources : fbd_enum_headers)

sources = [
  generated_dbus_sources,
  fbd_enum_sources,
  'fbd.c',
  'fbd-dev-vibra.c',
  'fbd-dev-sound.c',
  'fbd-event.c',
  'fbd-feedback-base.c',
  'fbd-feedback-dummy.c',
  'fbd-feedback-manager.c',
  'fbd-feedback-profile.c',
  'fbd-feedback-sound.c',
  'fbd-feedback-theme.c',
  'fbd-feedback-vibra.c',
  'fbd-feedback-vibra-periodic.c',
  'fbd-feedback-vibra-rumble.c',
]

fbd_deps = [
  gio,
  gio_unix,
  glib,
  gobject,
  gsound,
  gudev,
  json_glib,
]

fbd_inc = [
  include_directories('.'),
  libfeedback_inc,
  dbus_inc,
]

fbd_lib = static_library(
  'fbd',
  sources,
  include_directories : fbd_inc,
  dependencies : fbd_deps,
  install : false,
)

fbd_dep = declare_dependency(
  include_directories : fbd_inc,
  link_with : fbd_lib,
  dependencies : fbd_deps,
)

executable(
  'feedbackd',
  sources : 'fbd.c',
  include_directories : fbd_inc,
  dependencies : fbd_dep,
  install : true,
  install_dir: libexecdir,
)

endif
