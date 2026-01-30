project(
	'QmlGreet',
	['c', 'cpp'],
	version: '0.1',
	license: 'MIT',
	meson_version: '>=0.60.0',
	default_options: [
		'cpp_std=c++17',
		'warning_level=2',
		'werror=false',
	],
)

# Standard Qt6 Modules for QML
qt_mod = import('qt6')
qt_deps = dependency('qt6',
    modules: [
        'Core',
        'Gui',
        'Qml',
        'Quick',
        'QuickControls2',
        'WaylandClient',
        'DBus'
    ],
    required: true
)

# Qt6Core5Compat for Qt5Compat.GraphicalEffects
qt6_core5compat = dependency('Qt6Core5Compat', required: true)

# MauiKit4 library - required for QML plugin types to work
# The QML plugin needs the main library to be loaded for type registration
# Force link with --no-as-needed to ensure the library is loaded at runtime
mauikit_lib = declare_dependency(
    link_args: ['-Wl,--no-as-needed', '/usr/lib/x86_64-linux-gnu/libMauiKit4.so.4', '-Wl,--as-needed']
)

# Get Qt version for private headers path
qt6_core = dependency('qt6', modules: ['Core'])
qt6_version = qt6_core.version()
qt_private_include = include_directories('/usr/include/x86_64-linux-gnu/qt6/QtGui/@0@/QtGui'.format(qt6_version))
# Wayland Dependencies (for raw C access)
wl_client_dep = dependency('wayland-client')
wl_scanner = find_program('wayland-scanner')

# Define the Protocol Generator
# This converts .xml -> .c and .h
wl_scanner_code = generator(
    wl_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
    capture: false
)

wl_scanner_client_header = generator(
    wl_scanner,
    output: '@BASENAME@-client-protocol.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
    capture: false
)

# Generate the files for Layer Shell
layer_shell_xml = 'src/protocols/wlr-layer-shell-unstable-v1.xml'
xdg_shell_xml   = 'src/protocols/xdg-shell.xml' # Optional, but good to have

protocol_srcs = [
    wl_scanner_code.process(layer_shell_xml),
    wl_scanner_client_header.process(layer_shell_xml),
    wl_scanner_code.process(xdg_shell_xml),
    wl_scanner_client_header.process(xdg_shell_xml),
]

# Other Dependencies
# We keep these for low-level system access if needed later
thread_dep = dependency('threads')

# --- Source Files ---

# 1. The Entry Point
sources = [
    'src/main.cpp',
]

# 2. The Backend
sources += [
    'src/backend/AuthWrapper.cpp',
    'src/backend/SessionModel.cpp',
    'src/backend/UserModel.cpp',
    'src/backend/SystemPower.cpp',
    'src/backend/SystemBattery.cpp',
    'src/backend/LayerShell.cpp',
    'src/backend/ColorSchemeLoader.cpp',
]

# Process MOC headers for Qt meta-object system
moc_headers = [
    'src/backend/AuthWrapper.h',
    'src/backend/SessionModel.h',
    'src/backend/UserModel.h',
    'src/backend/SystemPower.h',
    'src/backend/SystemBattery.h',
    'src/backend/LayerShell.h',
    'src/backend/ColorSchemeLoader.h',
]

moc_files = qt_mod.preprocess(moc_headers: moc_headers)

# 3. Protocols (Keep existing protocols for Wayland Layer Shell)
# We will likely need to generate code for these later to ensure the window 
# stays "pinned" as a greeter.
wl_protocols = [
    'src/protocols/wlr-layer-shell-unstable-v1.xml',
    'src/protocols/xdg-shell.xml',
]

# --- Build Executable ---

# Process QML Resources (so you don't have to carry loose .qml files)
qml_resources = qt_mod.compile_resources(
    name: 'qml_resources',
    sources: 'resources.qrc'
)

executable(
    'qmlgreet',
    sources + protocol_srcs + moc_files,
    qml_resources,
    dependencies: [qt_deps, qt6_core5compat, thread_dep, wl_client_dep, mauikit_lib],
    include_directories: qt_private_include,
    install: true
)
