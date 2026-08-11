# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

project(kwayland-shell)

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets WaylandClient)

if(${QT_VERSION_MAJOR} STREQUAL "5")
    find_package(Qt5 REQUIRED COMPONENTS XkbCommonSupport)
endif()

include(wayland-shell.cmake)

try_compile(DEEPIN_KWIN_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR}/deepin-kwin-test
    ${CMAKE_CURRENT_LIST_DIR}/config.tests/deepin-kwin-test deepin-kwin-test CMAKE_FLAGS -DQT_VERSION_MAJOR=${QT_VERSION_MAJOR})

try_compile(WAYLAND_TEST_COMPILE_RESULT ${CMAKE_CURRENT_BINARY_DIR}/wayland_test
    ${CMAKE_CURRENT_LIST_DIR}/config.tests/wayland_test wayland_test CMAKE_FLAGS -DQT_VERSION_MAJOR=${QT_VERSION_MAJOR})

if(WAYLAND_TEST_COMPILE_RESULT)
    add_definitions(-DUSE_DEEPIN_WAYLAND)
    set(GLOBAL_HEADERS
        dwaylandshellmanager.h
        dkeyboard.h
        ${CMAKE_SOURCE_DIR}/src/global.h
        ${CMAKE_SOURCE_DIR}/src/vtablehook.h
    )
    set(GLOBAL_SOURCES
        dwaylandshellmanager.cpp
        dkeyboard.cpp
        ${CMAKE_SOURCE_DIR}/src/global.cpp
        ${CMAKE_SOURCE_DIR}/src/vtablehook.cpp
    )
else()
    message(WARNING "QtWayland version incompatible")
endif()

# Generate the wayland client C protocol header for com_deepin_client_management.
# The DWayland client library binds the global but does not expose the
# show_split_menu/hide_split_menu requests, so the generated C header is used to
# marshal those requests. wayland-scanner is a standard wayland build tool.
find_program(WAYLAND_SCANNER NAMES wayland-scanner)
if(WAYLAND_SCANNER)
    set(CLIENT_MANAGEMENT_XML ${CMAKE_CURRENT_LIST_DIR}/protocols/com-deepin-client-management.xml)
    set(CLIENT_MANAGEMENT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/wayland-client-com-deepin-client-management.h)
    add_custom_command(
        OUTPUT ${CLIENT_MANAGEMENT_HEADER}
        COMMAND ${WAYLAND_SCANNER} client-header ${CLIENT_MANAGEMENT_XML} ${CLIENT_MANAGEMENT_HEADER}
        DEPENDS ${CLIENT_MANAGEMENT_XML}
        VERBATIM
    )
else()
    message(WARNING "wayland-scanner not found; split screen menu (Wayland) will be unavailable")
endif()

add_library(${PROJECT_NAME} MODULE main.cpp ${GLOBAL_SOURCES} ${GLOBAL_HEADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/wayland-shell-integration
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    QT5DWAYLANDPLUGIN_LIBRARY
    QT_DEPRECATED_WARNINGS
)

if(DEEPIN_KWIN_TEST_COMPILE_RESULT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE D_DEEPIN_KWIN)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

list(APPEND COMMON_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WidgetsPrivate
    Qt${QT_VERSION_MAJOR}::WaylandClient
)
if(${QT_VERSION_MAJOR} STREQUAL "5")
    list(APPEND COMMON_LIBS Qt5::XkbCommonSupportPrivate Qt5::WaylandClientPrivate)
endif()

target_link_libraries(${PROJECT_NAME} ${COMMON_LIBS})

if(WAYLAND_SCANNER AND DWayland_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE ${CLIENT_MANAGEMENT_HEADER})
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(NOT INSTALL_PATH)
    set(INSTALL_PATH ${CMAKE_INSTALL_LIBDIR}/qt${QT_VERSION_MAJOR}/plugins/wayland-shell-integration)
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_PATH})
