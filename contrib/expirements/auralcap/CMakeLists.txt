cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)

project(auralcap)

find_package(Corrade REQUIRED Utility)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS OFF)

corrade_add_resource(Resources resources.conf)

add_executable(auralcap roboconductor.cc ${Resources})

target_link_libraries(auralcap PRIVATE
    csound64
    pthread
    event
    Corrade::Utility
)

install(TARGETS auralcap DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")