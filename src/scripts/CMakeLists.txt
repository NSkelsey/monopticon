project(monopt_iface_proto CXX)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE RELEASE)

install(PROGRAMS monopt_iface_proto.sh DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/ RENAME monopt_iface_proto)
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_PREFIX}/share/monopticon  FILES_MATCHING PATTERN "*.zeek")
