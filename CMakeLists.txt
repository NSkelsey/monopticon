project(monopticon CXX)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=gnu++14")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")


find_package(Protobuf)
find_package(pugixml)

set(WITH_IMGUI ON)

find_package(Corrade REQUIRED PluginManager Utility Containers)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

find_package(MagnumPlugins REQUIRED FreeTypeFont)

find_package(MagnumIntegration REQUIRED ImGui)
find_package(Magnum REQUIRED
                    EmscriptenApplication
                    GL
                    MeshTools
                    Primitives
                    SceneGraph
                    Shaders
                    Text
                    Trade
                    )


corrade_add_resource(monopticon_RESOURCES resources.conf)

set(monopticon_FILES
        contrib/expirements/ws/newproto/epoch.pb.h
        contrib/expirements/ws/newproto/epoch.pb.cc
        src/evenbettercap.h
        src/eth_device.cpp
        src/figure.cpp
        src/levelthree.cpp
        src/util.cpp
        src/context.cpp
        src/ws_context.cpp
        src/layout.cpp
        src/evenbettercap.cpp
    )

add_executable(monopticon ${monopticon_FILES} ${monopticon_RESOURCES})


target_link_libraries(monopticon PRIVATE
    pugixml
    websocket
    protobuf::libprotobuf-lite
    Corrade::Utility
    Corrade::Containers
    Magnum::Application
    Magnum::EmscriptenApplication
    Magnum::GL
    Magnum::Magnum
    Magnum::MeshTools
    Magnum::Primitives
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Text
    Magnum::Trade
    MagnumIntegration::ImGui
    MagnumPlugins::FreeTypeFont
)


target_compile_options(monopticon PRIVATE)

set_target_properties(monopticon
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS monopticon DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")
