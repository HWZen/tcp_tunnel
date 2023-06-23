//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#ifndef TCP_TUNNEL_ASIO_PCH_H
#define TCP_TUNNEL_ASIO_PCH_H
#ifndef ASIO_SEPARATE_COMPILATION
#warning "ASIO_SEPARATE_COMPILATION no define, please check CMakeLists.txt"
#endif // ASIO_SEPARATE_COMPILATION

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // win7
#endif

#include <asio/ip/tcp.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/detached.hpp>
#include <asio/read_until.hpp>
#include <asio/bind_allocator.hpp>
#include <asio/recycling_allocator.hpp>
using asio::awaitable;
using asio::buffer;
using asio::co_spawn;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;
using namespace asio::experimental::awaitable_operators;
using std::chrono::steady_clock;
inline constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
inline awaitable<void> timeout(steady_clock::duration duration)
{
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(use_nothrow_awaitable);
}





#endif //TCP_TUNNEL_ASIO_PCH_H
