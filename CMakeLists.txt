cmake_minimum_required(VERSION 3.24)
project(tcp_tunnel)

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(asio CONFIG REQUIRED)
add_definitions(-DASIO_SEPARATE_COMPILATION)
find_package(protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

include_directories(./)

add_executable(tcp_tunnel_server
        proto/net_pack.pb.cc
        server/server.cpp
        server/TunnelListener.cpp
        asio_impl.cpp
        server/Gateway.cpp
        server/Pool.cpp
        server/ClientProcess.cpp
        server/ClientProcess.cpp
        Logger.cpp)
add_executable(tcp_tunnel_client
        proto/net_pack.pb.cc
        Client/client.cpp
        asio_impl.cpp
        Client/Advertiser.cpp
        Client/Advertiser.h
        Client/Agent.cpp
        Logger.cpp)

target_link_libraries(tcp_tunnel_server PRIVATE asio::asio protobuf::libprotobuf spdlog::spdlog)
target_link_libraries(tcp_tunnel_client PRIVATE asio::asio protobuf::libprotobuf spdlog::spdlog)
