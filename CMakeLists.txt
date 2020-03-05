cmake_minimum_required(VERSION 3.14)
project(rpg)

find_package(PkgConfig REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sdl2-cmake-modules)

find_package(SDL2 REQUIRED)
pkg_check_modules(GUILE REQUIRED guile-2.2)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


add_executable(rpg src/main.cpp)
set_property(TARGET rpg PROPERTY CXX_STANDARD 20)

target_include_directories(rpg PUBLIC ${GUILE_INCLUDE_DIRS})
target_include_directories(rpg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/schmutz)

target_link_libraries(rpg ${CONAN_LIBS} ${SDL2_LIBRARIES} ${GUILE_LIBRARIES})
