cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)

project(MonoptAnimExample)

# Add module path in case this is project root
#if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
#    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../../modules/" ${CMAKE_MODULE_PATH})
#endif()

find_package(Magnum REQUIRED GL Shaders Sdl2Application SceneGraph)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

add_executable(anim2d anim2d.cpp)

link_directories("/usr/local/lib/")
target_link_libraries(anim2d PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::SceneGraph)

install(TARGETS anim2d DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
