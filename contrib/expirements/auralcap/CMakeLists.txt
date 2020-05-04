cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)

project(auralcap)

add_executable(robocond roboconductor.cpp)

target_link_libraries(robocond PRIVATE
    csound64
    pthread
    event
)

install(TARGETS robocond DESTINATION /usr/local/)