
project('flamethrower', 'c')

subdir('src')

cc = meson.get_compiler('c')
math = cc.find_library('m', required: false)

executable('flamethrower', sources,
    install: true,
    include_directories: include_directories('./third-party'),
    dependencies: math
)
