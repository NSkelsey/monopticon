cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE Debug)

project(test_websockets)

find_package(Corrade REQUIRED Utility)
set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS OFF)



add_executable(test_websocket_send test_websocket_send.c)

set_property(TARGET test_websocket_send APPEND_STRING PROPERTY
	    LINK_FLAGS_DEBUG " -s SOCKET_DEBUG=1")

target_link_libraries(test_websocket_send PRIVATE
		websocket)

install(TARGETS test_websocket_send DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/")
