//
// Created by HWZ on 2023/6/23.
//

#include "ClientProcess.h"


awaitable<void> ClientProcess::operator()() {
    constexpr size_t bufferSize{1024 * 1024 * 2};
    std::vector<char> buffer(bufferSize);


    for(;;){
        uint64_t recvLen{0};
        auto [ec, len] = co_await socket.async_read_some(asio::buffer(buffer), use_nothrow_awaitable);
        if (ec) {
            // log it
            break;
        }
        if (recvLen > bufferSize){
            // log it
            break;
        }

        auto [ec2, len2] = co_await socket.async_read_some(asio::buffer(buffer.data(), recvLen), use_nothrow_awaitable);
        if (ec2){
            // log it
            break;
        }

        net::data data;
        if (!data.ParseFromArray(buffer.data(), len2)){
            // log it
            continue;
        }

        if (data.has_listen_request()) {
            co_spawn(co_await this_coro::executor, ProcessRequest(data.listen_request()), asio::detached);
        }
        else if (data.has_pack()){
            co_spawn(co_await this_coro::executor, ProcessPackage(data.pack()), asio::detached);
        }
        else{
            // log it
        }


    }
}

awaitable<void> ClientProcess::ProcessRequest(const net::listen_request &request) {
    // TODO:
    co_return;
}

awaitable<void> ClientProcess::ProcessPackage(const net::pack &pack) {
    // TODO:
    co_return;
}
