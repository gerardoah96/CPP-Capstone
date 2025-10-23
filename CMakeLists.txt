cmake_minimum_required(VERSION 3.10)
project(FroggerGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -pedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

#  add this:
find_package(Threads REQUIRED)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

set(SOURCES
    src/game.cpp
    src/render.cpp
    src/frog.cpp
    src/vehicle.cpp
    src/lane.cpp
    src/main.cpp
)

add_executable(frogger ${SOURCES})

# link SDL2 + pthreads
target_link_libraries(frogger
    ${SDL2_LIBRARIES}
    Threads::Threads           #  this fixes the pthread_create undefined reference
)
