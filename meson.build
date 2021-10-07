project('ltree', 'c',
    version: '0.1.0',
    default_options: [
        'c_std=gnu11',
        'warning_level=3',
    ],
)

cc = meson.get_compiler('c')

python_snippet = '''
import sys, subprocess
subprocess.run(executable=sys.argv[1],
    args=[sys.argv[1]],
    stdin=open(sys.argv[2], "rb"),
    stdout=open(sys.argv[3], "wb"),
    shell=False)
'''

scdoc = find_program('scdoc', required: false, native: true)
if scdoc.found()
    python = import('python3').find_python()
    manpage = custom_target('manpage',
        build_always: true,
        input: 'ltree.1.scd',
        output: 'ltree.1',
        command: [python, '-c', python_snippet, scdoc, '@INPUT@', '@OUTPUT@'],
        install: true,
        install_dir: join_paths(get_option('mandir'), 'man1'),
    )
endif

executable('ltree', 'ltree.c',
    dependencies: dependency('libarchive'),
    include_directories: include_directories('deps'),
)
