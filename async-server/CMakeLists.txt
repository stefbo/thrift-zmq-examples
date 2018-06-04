# This file is part of the Thrift-ZeroMQ examples
#    (https://github.com/stefbo/thrift-zmq-examples).
# Copyright (c) 2018, Stefan Bolus.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)
project(async_server CXX)

set(ZMQ_LIBRARIES zmq)

set(THRIFT_EXECUTABLE thrift)
set(THRIFT_LIBRARIES thrift pthread)
set(THRIFT_LIBRARIES_ASYNC thrift pthread thriftnb event)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT ${GEN_DIR}/gen-cpp/service_types.h
	${GEN_DIR}/gen-cpp/service_types.cpp
	${GEN_DIR}/gen-cpp/service_constants.h
	${GEN_DIR}/gen-cpp/service_constants.cpp
	${GEN_DIR}/gen-cpp/Service.h
	${GEN_DIR}/gen-cpp/Service.cpp
	COMMAND ${THRIFT_EXECUTABLE} --gen cpp:cob_style ${CMAKE_CURRENT_SOURCE_DIR}/service.thrift
	WORKING_DIRECTORY ${GEN_DIR}
	DEPENDS service.thrift
)

add_executable(server
	src/server.cpp
	src/TAsyncZmqServer.cpp
	${GEN_DIR}/gen-cpp/service_types.cpp
	${GEN_DIR}/gen-cpp/service_constants.cpp
	${GEN_DIR}/gen-cpp/Service.cpp
)

target_include_directories(server PRIVATE ${GEN_DIR})

target_link_libraries(server ${THRIFT_LIBRARIES_ASYNC} ${ZMQ_LIBRARIES})

add_executable(client
	src/client.cpp
	src/TZmqClient.cpp
	${GEN_DIR}/gen-cpp/service_types.cpp
	${GEN_DIR}/gen-cpp/service_constants.cpp
	${GEN_DIR}/gen-cpp/Service.cpp
)

target_include_directories(client PRIVATE ${GEN_DIR})

target_link_libraries(client ${THRIFT_LIBRARIES} ${ZMQ_LIBRARIES})