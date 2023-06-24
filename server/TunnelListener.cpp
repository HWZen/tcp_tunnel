//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#include "TunnelListener.h"
#include "Pool.h"
#include <chrono>
using AcceptorPool = Pool<tcp::acceptor>;
using namespace std::chrono_literals;


awaitable<void> TunnelListener::ProcessPack(const net::pack& pack)
{
    switch (pack.type()){
    case net::pack_Type_connect:
        // client's ark
        co_spawn(co_await this_coro::executor, ResponseNewConnection(pack), asio::detached);
        break;
    case net::pack_Type_disconnect:{
        std::lock_guard ul{connectionMutex};
        connection.erase(pack.id());
        break;
    }
    case net::pack_Type_translate:
        co_spawn(co_await this_coro::executor, SendToConnection(pack), asio::detached);
        break;
    default:
        break;
    }
    co_return;
}

awaitable<void> TunnelListener::SendToConnection(const net::pack& pack)
{
    // check id exist
    std::unique_lock ul{connectionMutex};
    auto it = connection.find(pack.id());
    if (it == connection.end()) {
        ul.unlock();
        // log it
        // send client that this connection was disconnected
        net::pack response;
        response.set_id(pack.id());
        response.set_type(net::pack::Type::pack_Type_disconnect);

        co_await RequireSendToClient(response);

        co_return;
    }
    ul.unlock();

    // send to connection
    auto [ec, _] = co_await it->second.async_write_some(asio::buffer(pack.data()), use_nothrow_awaitable);
    if (ec){
        // log it
        // send client that this connection was disconnected
        net::pack response;
        response.set_id(pack.id());
        response.set_type(net::pack::Type::pack_Type_disconnect);

        co_await RequireSendToClient(response);

        // remove connection
        ul.lock();
        connection.erase(it);
    }

}

TunnelListener::TunnelListener(const asio::any_io_executor& io_context,
        uint16_t port,
        std::function<awaitable<void>(uint64_t)> NewConnection,
        std::function<awaitable<void>(const net::pack&)> SendToClient,
        std::function<void()> RequireDestroy
        ):
        port(port),
        RequireNewConnection(std::move(NewConnection)),
        RequireSendToClient(std::move(SendToClient)),
        RequireDestroy(std::move(RequireDestroy))
{
    co_spawn(io_context, [this]() -> awaitable<void> {
        for(;;) {
            auto acceptor = AcceptorPool::GetInstance().GetAcceptor(this->port);
            auto [ec, socket] = co_await acceptor->async_accept(use_nothrow_awaitable);
            if (ec) {
                // log it
                this->RequireDestroy();
            }
            auto this_cnt = cnt++;
            std::unique_lock ul{waitAckConnectionMutex};
            this->waitAckConnection.emplace(this_cnt, std::move(socket));
            ul.unlock();
            ul.release();

            co_spawn(co_await this_coro::executor, this->RequireNewConnection(this_cnt), asio::detached);
            co_spawn(co_await this_coro::executor, [this_cnt, this]() -> awaitable<void> {
                  co_await timeout(5s);
                  std::lock_guard lg{this->waitAckConnectionMutex};
                  this->waitAckConnection.erase(this_cnt);
            }, asio::detached);
        }
    }, asio::detached);

}

TunnelListener::~TunnelListener() {
    Pool<tcp::acceptor>::GetInstance().ReleaseAcceptor(port);
}

awaitable<void> TunnelListener::ResponseNewConnection(const net::pack &response) {
    // check if response port is in waitAckConnection or connection
    std::unique_lock ul1{waitAckConnectionMutex, std::defer_lock};
    std::unique_lock ul2{connectionMutex, std::defer_lock};
    std::lock(ul1, ul2);

    if (!waitAckConnection.contains(response.id()) && !connection.contains(response.id())) {
        // log it
        // send client to disconnect
        net::pack pack;
        pack.set_id(response.id());
        pack.set_port(response.port());
        pack.set_type(net::pack::Type::pack_Type_disconnect);
        ul1.unlock();
        ul2.unlock();
        co_await RequireSendToClient(pack);
        co_return ;
    }

    if (connection.contains(response.id())){
        // log it
        // unexplained circumstances
        co_return ;
    }

    // move socket from waitAckConnection to connection
    connection.insert(waitAckConnection.extract(response.id()));
}


