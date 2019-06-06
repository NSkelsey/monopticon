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
                    Trade
                    )

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(ebc_RESOURCES resources.conf)

set(ebc_FILES
        evenbettercap.h
        eth_device.cpp
        figure.cpp
        evenbettercap.cpp
    )

add_executable(ebc ${ebc_FILES} ${ebc_RESOURCES})
target_link_libraries(ebc PRIVATE
    broker
    caf_core
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Trade
    MagnumIntegration::ImGui
)


target_compile_options(ebc PRIVATE)

set_target_properties(ebc
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS ebc RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/ RENAME monopticon)

install(PROGRAMS monopt_iface_proto.sh DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/ RENAME monopt_iface_proto)
