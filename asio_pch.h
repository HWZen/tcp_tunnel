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

inline auto MakeSendSeq(uint64_t* len, auto&& proto_msg){
    return std::array<asio::const_buffer, 2>{
        asio::buffer(len, sizeof(*len)),
        asio::buffer(&proto_msg, *len)
    };
}

inline awaitable<std::tuple<asio::error_code, std::vector<char>>> RecvMsg(auto& socket){
    uint64_t len;
    auto [ec1, len1] = co_await socket.async_read_some(asio::buffer(&len, sizeof(len)), use_nothrow_awaitable);
    if (ec1){
        co_return std::make_tuple(ec1, std::vector<char>{});
    }
    std::vector<char> buf(static_cast<size_t>(len));
    auto [ec2, len2] = co_await socket.async_read_some(asio::buffer(buf), use_nothrow_awaitable);
    if (ec2){
        co_return std::make_tuple(ec2, std::vector<char>{});
    }
    co_return std::make_tuple(asio::error_code{}, std::move(buf));
}

constexpr size_t bufferSize{1024 * 1024 * 2};




#endif //TCP_TUNNEL_ASIO_PCH_H
