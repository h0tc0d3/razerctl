project('razerctl', 'c',
    version : '1.0.0',
    default_options : [
        'warning_level=3',
        'c_std=gnu17'
    ]
)

source_files = ['src/main.c', 'src/razer.c']

link_dependencies = []

compiler_args = [
    '-fstack-protector-strong'
]

link_args = [
    '-fstack-protector-strong'
]

cc = meson.get_compiler('c')
platform = target_machine.system()
cpu_family = target_machine.cpu_family()

hidraw = false
libusb_static = get_option('libusb_static')

if platform == 'linux'
    hidraw = get_option('hidraw')
endif

if platform == 'windows'

    compiler_args += [
        '-DNOMINMAX',
        '-D_WIN32_WINNT=0xa00',
    ]

    link_args += [
        '-static',
        '-static-libgcc',
        # We need to set the section alignment for debug symbols to
        # work properly as well as avoiding a memcpy from the Wine loader.
        '-Wl,--file-alignment=4096',
    ]

    # Wine's built-in back traces only work with dwarf4 symbols
    if get_option('debug')
        compiler_args += '-gdwarf-4'
    endif

    # Enable stdcall fixup on 32-bit
    if cpu_family == 'x86'
        link_args += [
        '-Wl,--enable-stdcall-fixup',
        '-Wl,--kill-at'
        ]
    endif

    libusb_static = true
    hidraw = false

endif

add_project_arguments(cc.get_supported_arguments(compiler_args), language: 'c')
add_project_link_arguments(cc.get_supported_link_arguments(link_args), language: 'c')

if hidraw
    source_files += 'src/hidraw.c'
    libudev = dependency('libudev', required: true)
    link_dependencies += libudev
    add_project_arguments('-DRAZERCTL_USE_HIDRAW', language: 'c')
endif

if not hidraw or get_option('libusb')
    libusb = dependency('libusb-1.0', static: libusb_static)
    if libusb.found()
        source_files += 'src/libusb.c'
        link_dependencies += libusb
        add_project_arguments('-DRAZERCTL_USE_LIBUSB', language: 'c')
    endif
endif

executable('razerctl', source_files, dependencies: link_dependencies, install: true)
