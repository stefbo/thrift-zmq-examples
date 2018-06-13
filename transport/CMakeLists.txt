cmake_minimum_required(VERSION 3.5)
project(async_thrift_0mq_server CXX)

set(ZMQ_LIBRARIES zmq)

set(THRIFT_EXECUTABLE thrift)
set(THRIFT_LIBRARIES thrift pthread)

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT ${GEN_DIR}/gen-cpp/example_types.h
	${GEN_DIR}/gen-cpp/example_types.cpp
	${GEN_DIR}/gen-cpp/example_constants.h
	${GEN_DIR}/gen-cpp/example_constants.cpp
	${GEN_DIR}/gen-cpp/Example.h
	${GEN_DIR}/gen-cpp/Example.cpp
	COMMAND ${THRIFT_EXECUTABLE} --gen cpp ${CMAKE_CURRENT_SOURCE_DIR}/example.thrift
	WORKING_DIRECTORY ${GEN_DIR}
	DEPENDS example.thrift
)

add_executable(client
	client.cpp
	TZmqTransport.cpp
	${GEN_DIR}/gen-cpp/example_types.cpp
	${GEN_DIR}/gen-cpp/example_constants.cpp
	${GEN_DIR}/gen-cpp/Example.cpp
)

target_include_directories(client PRIVATE ${GEN_DIR})
target_link_libraries(client ${THRIFT_LIBRARIES} ${ZMQ_LIBRARIES})

add_executable(server
	server.cpp
	TZmqServer.cpp
	TZmqTransport.cpp
	${GEN_DIR}/gen-cpp/example_types.cpp
	${GEN_DIR}/gen-cpp/example_constants.cpp
	${GEN_DIR}/gen-cpp/Example.cpp
)

target_include_directories(server PRIVATE ${GEN_DIR})
target_link_libraries(server ${THRIFT_LIBRARIES} ${ZMQ_LIBRARIES})
