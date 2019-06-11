project(monopticon CXX)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")

find_package(Broker REQUIRED)


find_package(MagnumIntegration REQUIRED ImGui)
find_package(Magnum REQUIRED
                    GL
                    MeshTools
                    Primitives
                    SceneGraph
                    Sdl2Application
                    Shaders
                    Text
                    Trade
                    )

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(monopticon_RESOURCES resources.conf)

set(monopticon_FILES
        evenbettercap.h
        eth_device.cpp
        figure.cpp
        evenbettercap.cpp
    )

add_executable(monopticon ${monopticon_FILES} ${monopticon_RESOURCES})

# included to reach libbroker and libcaf installed in non default locations
link_directories("/usr/local/lib/")
target_link_libraries(monopticon PRIVATE
    broker
    caf_core
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Text
    Magnum::Trade
    MagnumIntegration::ImGui
)


target_compile_options(monopticon PRIVATE)

set_target_properties(monopticon
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS monopticon DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")

install(PROGRAMS monopt_iface_proto.sh DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/ RENAME monopt_iface_proto)
install(DIRECTORY src/scripts DESTINATION ${CMAKE_INSTALL_PREFIX}/share/monopticon  FILES_MATCHING PATTERN "*.bro")
