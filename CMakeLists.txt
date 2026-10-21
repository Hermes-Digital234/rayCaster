cmake_minimum_required(VERSION 3.16)
project(raycaster C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(raycaster
  src/main.c
  src/map.c
  src/input.c
  src/player.c
  src/raycaster.c
)

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

# Copy the default map next to the executable so it runs from the build folder.
# Looks in the project root first, then src/. Skipped (with a note) if neither exists.
set(MAP_IMAGE "")
foreach(candidate "${CMAKE_CURRENT_SOURCE_DIR}/map.png" "${CMAKE_CURRENT_SOURCE_DIR}/src/map.png")
  if(NOT MAP_IMAGE AND EXISTS "${candidate}")
    set(MAP_IMAGE "${candidate}")
  endif()
endforeach()

if(MAP_IMAGE)
  add_custom_command(TARGET raycaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MAP_IMAGE}" $<TARGET_FILE_DIR:raycaster>/map.png
  )
else()
  message(STATUS "map.png not found; pass a map path when running: ./raycaster yourmap.png")
endif()