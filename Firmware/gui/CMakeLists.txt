cmake_minimum_required(VERSION 3.23)

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_C_FLAGS_DEBUG "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O3")

project(brass_gui)

find_package(Freetype REQUIRED)

include_directories(.)
include_directories(third-party/rapidxml)
include_directories(third-party/utf8)
include_directories(${FREETYPE_INCLUDE_DIRS})

set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Build LVGL examples" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Build LVGL demos" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Build LVGL internal ThorVG" FORCE)

add_subdirectory(lvgl EXCLUDE_FROM_ALL)
add_subdirectory(src)

install(TARGETS ${PROJECT_NAME} DESTINATION sbin)
install(TARGETS lvgl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY rootfs/ DESTINATION / USE_SOURCE_PERMISSIONS)
