# tcp_tunnel
A simple TCP tunnel based cpp20 and asio

## Why
Do you have a PC in your home and want to advertise a server (e.g., Windows RDP), 
but you don't have a public IP address?

Tcp_tunnel can help you.

You can think of it as a special proxy server.

Normally, a proxy client translates requests to the proxy server, 
and the proxy server visits the requested destination on your behalf. 
This allows you to access places that you normally wouldn't be able to access (e.g., google.com).

However, Tcp_tunnel works differently. It doesn't proxy your requests; instead, it proxies your service.
You can use the tcp_tunnel client to request the tcp_tunnel server to listen on a specific port. 
When the server receives a request, it will forward it to the client.

In other words, the tcp_tunnel server listens on your behalf.

In general, Tcp_tunnel can help you expose your service to the Internet and make it accessible.

## Build
Tcp_tunnel based on cmake.
```shell
mkdir build
cd build
cmake ../ -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --buld .
```

## require
- C++20
- vcpkg
- asio (auto installed)
- google protobuf (auto installed)
