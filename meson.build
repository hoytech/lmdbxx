project(
  'lmdbxx',
  'cpp',
  version : '1.0.0',
  license: 'Unlicense',
  default_options : ['warning_level=3', 'cpp_std=c++17']
)

lmdb_dep = dependency('lmdb')

lmdbxx_dep = declare_dependency(include_directories : 'include/', dependencies: lmdb_dep)
meson.override_dependency('lmdb++', lmdbxx_dep)

install_headers('lmdb++.h')

pkg = import('pkgconfig')
pkg.generate(libraries: lmdbxx_dep, name: 'lmdb++', description: 'C++17 wrapper for the LMDB embedded B+ tree database library')

if get_option('examples')
  check = executable(
    'example',
    'example.cc',
    dependencies: lmdbxx_dep,
    install: false
  )
endif

if get_option('tests')
  check = executable(
    'check',
    'check.cc',
    dependencies: lmdbxx_dep,
    install: false
  )

  test('check', check)
endif

