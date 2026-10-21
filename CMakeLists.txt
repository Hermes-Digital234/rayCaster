cmake_minimum_required(VERSION 3.16)
project(raycaster C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Every .c under src/ (new files are picked up automatically on the next build)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_executable(raycaster ${SOURCES})

# Includes are written relative to src/, e.g. #include "core/stateManager.h"
target_include_directories(raycaster PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(raycaster PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
)

# raylib: use its CMake config if installed, otherwise fall back to pkg-config
find_package(raylib QUIET)
if(raylib_FOUND)
  target_link_libraries(raycaster PRIVATE raylib)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(RAYLIB REQUIRED raylib)
  target_include_directories(raycaster PRIVATE ${RAYLIB_INCLUDE_DIRS})
  target_link_directories(raycaster PRIVATE ${RAYLIB_LIBRARY_DIRS})
  target_link_libraries(raycaster PRIVATE ${RAYLIB_LIBRARIES})
endif()

if(NOT WIN32)
  target_link_libraries(raycaster PRIVATE m)
endif()

# Copy assets/ next to the executable on EVERY build, even when no code changed,
# so a new map in assets/maps shows up after a plain `cmake --build .`
add_custom_target(copyAssets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:raycaster>/assets
)
# copyAssets runs after raycaster is built (it already depends on it through TARGET_FILE_DIR)
add_dependencies(copyAssets raycaster)