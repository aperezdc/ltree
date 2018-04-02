project('ltree', 'c',
    version: '0.1.0',
    default_options: ['c_std=gnu11'],
)

cc = meson.get_compiler('c')
add_project_arguments(cc.get_supported_arguments(['-Wall']), language: 'c')

executable('ltree', 'ltree.c',
    dependencies: dependency('libarchive'),
    include_directories: include_directories('deps'),
)
