# Feedback daemon
if get_option('daemon')

fbd_enum_headers = files([
  'fbd-event.h',
  'fbd-feedback-led.h',
  'fbd-feedback-vibra.h',
])
fbd_enum_sources = gnome.mkenums_simple('fbd-enums',
  sources : fbd_enum_headers)

sources = [
  generated_dbus_sources,
  fbd_enum_sources,
  'fbd-error.c',
  'fbd-binder.c',
  'fbd-droid-vibra-backend.c',
  'fbd-droid-vibra-backend-hidl.c',
  'fbd-droid-vibra-backend-aidl.c',
  'fbd-droid-vibra-backend-sysfs.c',
  'fbd-droid-vibra.c',
  'fbd-droid-leds-backend.c',
  'fbd-droid-leds-backend-hidl.c',
  'fbd-droid-leds-backend-aidl.c',
  'fbd-droid-leds-backend-sysfs.c',
  'fbd-droid-leds.c',
  'fbd-dev-vibra.c',
  'fbd-dev-sound.c',
  'fbd-dev-led.c',
  'fbd-dev-led-multicolor.c',
  'fbd-dev-leds.c',
  'fbd-event.c',
  'fbd-feedback-base.c',
  'fbd-feedback-dummy.c',
  'fbd-feedback-led.c',
  'fbd-feedback-manager.c',
  'fbd-feedback-profile.c',
  'fbd-feedback-sound.c',
  'fbd-feedback-theme.c',
  'fbd-feedback-vibra.c',
  'fbd-feedback-vibra-periodic.c',
  'fbd-feedback-vibra-rumble.c',
  'fbd-theme-expander.c',
  'fbd-udev.c',
]

fbd_deps = [
  gio,
  gio_unix,
  glib,
  gmobile,
  gobject,
  gsound,
  gudev,
  json_glib,
  dependency('libgbinder'),
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
  sources: generated_dbus_sources,
  include_directories : fbd_inc,
  link_with : fbd_lib,
  dependencies : fbd_deps,
)

executable(
  'feedbackd',
  sources : ['fbd.c', generated_dbus_sources[1]],
  include_directories : fbd_inc,
  dependencies : fbd_dep,
  install : true,
  install_dir: libexecdir,
)

executable(
  'fbd-ledctrl',
  sources : ['fbd-ledctrl.c'],
  include_directories : fbd_inc,
  dependencies : [glib],
  install : true,
  install_dir: libexecdir,
)

executable(
  'fbd-theme-validate',
  sources : ['fbd-theme-validate.c'],
  include_directories : fbd_inc,
  dependencies : fbd_dep,
  install : true,
  install_dir: bindir,
)

endif
