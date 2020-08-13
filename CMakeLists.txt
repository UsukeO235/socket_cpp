cmake_minimum_required( VERSION 3.15 )

enable_language( CXX )
add_definitions( "-std=c++11" )

project( socket_cpp_examples )

include_directories(
	${PROJECT_SOURCE_DIR}/include
)

add_executable( server
	examples/polling/server.cpp
)

add_executable( client
	examples/polling/client.cpp
)