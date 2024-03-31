cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/")

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto transport_router.proto graph.proto)

set(CATALOG_SRC main.cpp json.cpp json_builder.cpp json_reader.cpp map_renderer.cpp
    request_handler.cpp svg.cpp transport_catalogue.cpp transport_router.cpp serialization.cpp)
set(CATALOG_HEADERS domain.h geo.h graph.h json.h json_builder.h json_reader.h map_renderer.h
    ranges.h request_handler.h router.h svg.h transport_catalogue.h transport_router.h serialization.h)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${CATALOG_SRC} ${CATALOG_HEADERS})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobufd.lib" "protobuf.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_RELEASE}")
string(REPLACE "protobufd.a" "protobuf.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_RELEASE}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY_RELEASE}>" Threads::Threads)
