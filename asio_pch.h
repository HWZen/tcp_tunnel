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

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // win7
#endif
#endif

#include <asio/ip/tcp.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/detached.hpp>
#include <asio/read_until.hpp>
#include <asio/bind_allocator.hpp>
#include <asio/recycling_allocator.hpp>
#include <asio/read.hpp>
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

struct SendSequence : public std::array<asio::const_buffer, 2> {
    uint64_t len;
    std::string buf;
    void ref(){
        this->operator[](0) = asio::buffer(&len, sizeof(len));
        this->operator[](1) = asio::buffer(buf);
    }
};


inline auto MakeSendSeq(auto&& proto_msg){
    SendSequence res;
    res.len = proto_msg.ByteSizeLong();
    res.buf = proto_msg.SerializeAsString();
    return res;
}


inline awaitable<std::tuple<asio::error_code, uint64_t>> SendMsg(auto& socket, auto&& proto_msg){
    SendSequence bufferSeq;
    bufferSeq.len = proto_msg.ByteSizeLong();
    bufferSeq.buf = proto_msg.SerializeAsString();
    bufferSeq[0] = asio::buffer(&bufferSeq.len, sizeof(bufferSeq.len));
    bufferSeq[1] = asio::buffer(bufferSeq.buf);
    auto [ec, len] = co_await socket.async_write_some(bufferSeq, use_nothrow_awaitable);
    co_return std::make_tuple(ec, len);
}



// Modified RecvMsg function to handle message boundaries correctly.
inline awaitable<std::tuple<asio::error_code, std::vector<char>>> RecvMsg(asio::ip::tcp::socket& socket) {
    // Read the length prefix (8 bytes).
    uint64_t len;
    std::vector<char> lenBuffer(sizeof(len));
    size_t totalBytesRead = 0;
    while (totalBytesRead < sizeof(len)) {
        auto buffer = asio::buffer(lenBuffer.data() + totalBytesRead, sizeof(len) - totalBytesRead);
        auto [ec, bytesRead] = co_await asio::async_read(socket, buffer, use_nothrow_awaitable);
        if (ec) {
            co_return std::make_tuple(ec, std::vector<char>{});
        }
        totalBytesRead += bytesRead;
    }
    // Now, we have the complete length prefix.
    len = *reinterpret_cast<uint64_t*>(lenBuffer.data());

    // Read the complete message data based on the received length.
    std::vector<char> buf(static_cast<size_t>(len));
    totalBytesRead = 0;
    while (totalBytesRead < len) {
        auto buffer = asio::buffer(buf.data() + totalBytesRead, len - totalBytesRead);
        auto [ec, bytesRead] = co_await asio::async_read(socket, buffer, use_nothrow_awaitable);
        if (ec) {
            co_return std::make_tuple(ec, std::vector<char>{});
        }
        totalBytesRead += bytesRead;
    }

    co_return std::make_tuple(asio::error_code{}, std::move(buf));
}

constexpr size_t bufferSize{1024 * 1024 * 2};




#endif //TCP_TUNNEL_ASIO_PCH_H
