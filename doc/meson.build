if get_option('gtk_doc')

expand_content_md_files = [
  'overview.md',
  'build-howto.md',
  'initialization.md',
  'Event-naming-spec-0.0.0.md',
  'Feedback-theme-spec-0.0.0.md',
]

toml_data = configuration_data()
toml_data.set('VERSION', meson.project_version())

libfeedback_toml = configure_file(
  input: 'libfeedback.toml.in',
  output: 'libfeedback.toml',
  configuration: toml_data
)

dependency('gi-docgen', version: '>= 2021.1',
           fallback: ['gi-docgen', 'dummy_dep'],
           native: true,
           required: get_option('gtk_doc'))

gidocgen = find_program('gi-docgen')

docs_dir = datadir / 'doc'

custom_target('libfeedback-doc',
  input: [ libfeedback_toml, libfeedback_gir[0] ],
  output: 'libfeedback-0',
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../src'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depend_files: [ expand_content_md_files ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
)

endif

if get_option('man')
  manpages = [
    'fbcli',
    'feedbackd',
  ]

  xsltproc = find_program('xsltproc', required : true)
  xsltproc_command = [
    xsltproc,
    '--nonet',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'man.th.extra1.suppress', '1',
    '--stringparam', 'man.authors.section.enabled', '0',
    '--stringparam', 'man.copyright.section.enabled', '0',
    '-o', '@OUTPUT@',
    'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
    '@INPUT@',
  ]
  man1_dir = get_option('mandir') + '/man1'

  foreach page : manpages
    custom_target(page + '-man',
      input  : page + '.xml',
      output : page + '.1',
      command: xsltproc_command,
      install: true,
      install_dir: man1_dir)
  endforeach
endif
