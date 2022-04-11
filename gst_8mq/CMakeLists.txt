cmake_minimum_required(VERSION 3.1)
project(gstreamer_issue1)

set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "Install path" FORCE)
set(C4_INSTALL_PREFIX "opt/control4/")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

add_subdirectory(ocl)

include_directories(
        ${GST_INCLUDE_DIRS}
        "/sdk/sysroots/aarch64-control4-linux/usr/include"
        "ocl"
)

add_executable(gstreamer_issue1
        main.cc)

target_link_libraries(gstreamer_issue1
        ${GST_LIBRARIES}
        gstallocators-1.0
        pthread
        GPU
)