cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Release)

project(PortManifold CXX)

# Add module path in case this is project root
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../../../modules/" ${CMAKE_MODULE_PATH})
endif()

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED GL Shaders Sdl2Application SceneGraph)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

add_executable(manifold WIN32 manifold.cpp)

target_link_libraries(manifold PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::SceneGraph)

install(TARGETS manifold DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
