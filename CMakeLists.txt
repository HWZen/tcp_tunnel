cmake_minimum_required(VERSION 3.24)
project(tcp_tunnel)

set(CMAKE_CXX_STANDARD 20)


find_package(asio CONFIG REQUIRED)
add_definitions(-DASIO_SEPARATE_COMPILATION)
find_package(protobuf CONFIG REQUIRED)

include_directories(./)

add_executable(tcp_tunnel_server proto/net_pack.pb.cc server/server.cpp server/TunnelListener.cpp asio_impl.cpp server/Gateway.cpp server/Pool.cpp server/Pool.h server/ClientProcess.cpp server/ClientProcess.h server/ClientProcess.cpp)
add_executable(tcp_tunnel_client proto/net_pack.pb.cc client.cpp asio_impl.cpp)

target_link_libraries(tcp_tunnel_server PRIVATE asio::asio protobuf::libprotobuf)
target_link_libraries(tcp_tunnel_client PRIVATE asio::asio protobuf::libprotobuf)
