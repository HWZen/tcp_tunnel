//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#include "TunnelListener.h"
#include <chrono>
using namespace std::chrono_literals;


awaitable<void> TunnelListener::ClientCallback(const net::pack& pack)
{
    switch (pack.type()){
    case net::pack_Type_connect:
        // client's ark
        break;
    case net::pack_Type_disconnect:
        connection.erase(pack.id());
        break;
    case net::pack_Type_translate:
        SendToConnection(pack);
        break;
    default:
        break;
    }
    co_return;
}

awaitable<void> TunnelListener::SendToConnection(const net::pack& pack)
{
    // check id exist
    auto it = connection.find(pack.id());
    if (it == connection.end()) {
        // log it
        // send client that this connection was disconnected
        net::pack response;
        response.set_id(pack.id());
        response.set_type(net::pack::Type::pack_Type_disconnect);

        RequireSendToClient(response);

        co_return;
    }

    // set buffers sequence
    std::vector<asio::const_buffer> buffers;
    uint64_t len = pack.ByteSizeLong();
    buffers.emplace_back(asio::buffer(&len, sizeof(len)));
    buffers.emplace_back(asio::buffer(&pack, len));

    // send to connection
    auto [ec, _] = co_await it->second.async_write_some(buffers, use_nothrow_awaitable);
    if (ec){
        // log it
        // send client that this connection was disconnected
        net::pack response;
        response.set_id(pack.id());
        response.set_type(net::pack::Type::pack_Type_disconnect);

        RequireSendToClient(response);

        // remove connection
        connection.erase(it);
    }

}

TunnelListener::TunnelListener(asio::io_context& io_context, tcp::acceptor acceptor,
        std::function<awaitable<void>(tcp::socket)> NewConnection,
        std::function<awaitable<void>(const net::pack&)> SendToClient,
        std::function<void()> RequireDestroy
        ):
        acceptor(std::move(acceptor)),
        RequireNewConnection(std::move(NewConnection)),
        RequireSendToClient(std::move(SendToClient)),
        RequireDestroy(std::move(RequireDestroy))
{
    co_spawn(io_context, [this]() -> awaitable<void> {
        for(;;) {
            auto [ec, socket] = co_await this->acceptor.async_accept(use_nothrow_awaitable);
            if (ec) {
                // log it
                this->RequireDestroy();
            }
            auto this_cnt = cnt++;
            this->waitAckConnection.emplace(this_cnt, std::move(socket));

            co_spawn(co_await this_coro::executor, this->RequireNewConnection(std::move(socket)), asio::detached);
            co_spawn(co_await this_coro::executor, [this_cnt, this]() -> awaitable<void> {
                  co_await timeout(5s);
                  this->waitAckConnection.erase(this_cnt);
            }, asio::detached);
        }
    }, asio::detached);


}


