cmake_minimum_required(VERSION 3.24)
project(tcp_tunnel)

set(CMAKE_CXX_STANDARD 20)


find_package(asio CONFIG REQUIRED)
add_definitions(-DASIO_SEPARATE_COMPILATION)
find_package(protobuf CONFIG REQUIRED)

add_executable(tcp_tunnel_server proto/net_pack.pb.cc server.cpp TunnelListener.cpp asio_impl.cpp Gateway.cpp)
add_executable(tcp_tunnel_client proto/net_pack.pb.cc client.cpp asio_impl.cpp)

target_link_libraries(tcp_tunnel_server PRIVATE asio::asio protobuf::libprotobuf)
target_link_libraries(tcp_tunnel_client PRIVATE asio::asio protobuf::libprotobuf)
