cmake_minimum_required(VERSION 3.10)

file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules")

find_package(Protobuf REQUIRED)
find_package(Broker REQUIRED)
include_directories(BEFORE ${BROKER_INCLUDE_DIR})
include_directories(BEFORE ${Protobuf_INCLUDE_DIRS})



init_target (mux_server)

build_executable_me (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

#link_boost ()

final_target ()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")
