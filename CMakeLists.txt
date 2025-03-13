cmake_minimum_required(VERSION 3.5)

project(bfd2)

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/logger/logger" 
    "logger"
)

add_executable(
 	bfd
	mainbfd.cpp
 	bfd.hpp
	bfd-dbus-server.hpp
)
find_package(sdbus-c++ REQUIRED)

target_compile_options(bfd PRIVATE
    -g3
	-O0
)

target_link_libraries(bfd PRIVATE SDBusCpp::sdbus-c++ logger)
set_target_properties(bfd PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

#add_executable(
#	bfd-dbus-client
#	client/bfd-dbus-client.cpp
#	client/bfd-dbus-client.hpp
#)
target_link_libraries(bfd-dbus-client PRIVATE SDBusCpp::sdbus-c++)
set_target_properties(bfd-dbus-client PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
