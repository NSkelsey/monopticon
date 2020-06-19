
file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.hpp)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")

find_package(Broker REQUIRED)
include_directories(BEFORE ${BROKER_INCLUDE_DIR})


set(CMAKE_BUILD_TYPE Debug)
init_target (mux_server)

build_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

#link_boost ()

set(CMAKE_BUILD_TYPE Debug)
target_link_libraries(${TARGET_NAME} PRIVATE
    protobuf
    "${BROKER_LIBRARY}"
    "${BROKER_ROOT_DIR}/lib/libcaf_core.so"
)

final_target ()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")
