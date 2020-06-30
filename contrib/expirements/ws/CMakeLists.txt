project(test_websockets)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)


find_package(Corrade REQUIRED Utility)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS OFF)


set(websock_FILES
	/usr/local/include/google/
	newproto/epoch.pb.h
	newproto/epoch.pb.cc
	websock_proto.cpp
)

add_executable(websock_proto ${websock_FILES})

set_property(TARGET websock_proto APPEND_STRING PROPERTY
	    LINK_FLAGS_DEBUG " -s SOCKET_DEBUG=1")

target_link_directories(websock_proto PRIVATE
	~/s/.libs
	)

target_link_libraries(websock_proto PRIVATE
		websocket
		protobuf
		)

install(TARGETS websock_proto DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")
