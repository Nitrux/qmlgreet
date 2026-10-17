cmake_minimum_required(VERSION 3.24)

project(qmlgreet VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CheckCXXCompilerFlag)
if(NOT CMAKE_CROSSCOMPILING)
    string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" QMLGREET_HOST_PROCESSOR)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" QMLGREET_TARGET_PROCESSOR)
    if(QMLGREET_HOST_PROCESSOR MATCHES "^(x86_64|amd64)$"
       AND QMLGREET_TARGET_PROCESSOR MATCHES "^(x86_64|amd64)$")
        check_cxx_compiler_flag("-march=x86-64-v3" QMLGREET_HAS_X86_64_V3)
        if(QMLGREET_HAS_X86_64_V3)
            add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-march=x86-64-v3>)
        endif()
    endif()
endif()

set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL
    "MauiKit uses Qt's private WaylandClient API" FORCE)

find_package(ECM 6.13 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(FeatureSummary)

find_package(Qt6 6.9 REQUIRED COMPONENTS
    Core Core5Compat DBus Gui GuiPrivate Qml Quick QuickControls2 WaylandClient)
find_package(KF6 6.13 REQUIRED COMPONENTS I18n)
find_package(MauiKit4 4.0.5 REQUIRED)
find_package(Threads REQUIRED)
find_package(Wayland REQUIRED COMPONENTS Client)
ecm_find_qmlmodule(org.mauikit.controls 1.0)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(GENERATED_PROTOCOL_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(WAYLAND_PROTOCOL_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocols/wlr-layer-shell-unstable-v1.xml"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocols/xdg-shell.xml")
set(WAYLAND_PROTOCOL_SOURCES)
set(WAYLAND_PROTOCOL_HEADERS)

foreach(protocol IN LISTS WAYLAND_PROTOCOL_FILES)
    get_filename_component(protocol_name "${protocol}" NAME_WE)
    set(protocol_source "${GENERATED_PROTOCOL_DIR}/${protocol_name}-protocol.c")
    set(protocol_header "${GENERATED_PROTOCOL_DIR}/${protocol_name}-client-protocol.h")
    add_custom_command(
        OUTPUT "${protocol_source}" "${protocol_header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_PROTOCOL_DIR}"
        COMMAND "${WAYLAND_SCANNER}" private-code "${protocol}" "${protocol_source}"
        COMMAND "${WAYLAND_SCANNER}" client-header "${protocol}" "${protocol_header}"
        DEPENDS "${protocol}")
    list(APPEND WAYLAND_PROTOCOL_SOURCES "${protocol_source}")
    list(APPEND WAYLAND_PROTOCOL_HEADERS "${protocol_header}")
endforeach()

add_executable(qmlgreet
    src/main.cpp
    src/backend/AuthWrapper.cpp
    src/backend/AuthWrapper.h
    src/backend/LayerShell.cpp
    src/backend/LayerShell.h
    src/backend/SessionModel.cpp
    src/backend/SessionModel.h
    src/backend/SystemBattery.cpp
    src/backend/SystemBattery.h
    src/backend/SystemPower.cpp
    src/backend/SystemPower.h
    src/backend/UserModel.cpp
    src/backend/UserModel.h
    resources.qrc
    ${WAYLAND_PROTOCOL_SOURCES}
    ${WAYLAND_PROTOCOL_HEADERS})

target_include_directories(qmlgreet PRIVATE "${GENERATED_PROTOCOL_DIR}")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(qmlgreet PRIVATE
        -UQT_NO_CAST_FROM_ASCII
        -UQT_NO_CAST_FROM_BYTEARRAY)
endif()
target_link_options(qmlgreet PRIVATE -Wl,--no-as-needed)
target_link_libraries(qmlgreet PRIVATE
    Qt6::Core
    Qt6::Core5Compat
    Qt6::DBus
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WaylandClient
    KF6::I18n
    KF6::I18nQml
    MauiKit4
    Threads::Threads
    Wayland::Client)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(qmlgreet PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

install(TARGETS qmlgreet RUNTIME DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES qmlgreet.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/qmlgreet)

if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
