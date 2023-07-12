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
    TRACE_FUNC(logger);
    switch (pack.type()){
    case net::pack_Type_connect:
        // client's ark
        co_spawn(co_await this_coro::executor, ResponseNewConnection(pack), asio::detached);
        break;
    case net::pack_Type_disconnect:
    {
        LOG_INFO(logger, "remove connection id:{}", pack.id());
        std::unique_lock ul{connectionMutex, std::defer_lock};
        std::unique_lock ul2{waitAckConnectionMutex, std::defer_lock};
        std::lock(ul, ul2);
        waitAckConnection.erase(pack.id());
        connection.erase(pack.id());
        break;
    }
    case net::pack_Type_translate:
        co_spawn(co_await this_coro::executor, SendToConnection(pack), asio::detached);
        break;
    case net::pack::ping:
    {
        LOG_INFO(logger, "recv ping");
        net::pack pong;
        pong.set_type(net::pack::pong);
        pong.set_port(port);
        co_spawn(co_await this_coro::executor, RequireSendToClient(pong), asio::detached);
    }
        break;
    default:
        LOG_WARN(logger, "unknown pack type:{}", pack.type());
        break;
    }
    co_return;
}

awaitable<void> TunnelListener::SendToConnection(net::pack pack)
{
    TRACE_FUNC(logger);
    // check id exist
    std::unique_lock ul{connectionMutex};
    auto it = connection.find(pack.id());
    if (it == connection.end()) {
        ul.unlock();
        LOG_ERROR(logger, "id:{} not found", pack.id());
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
        LOG_ERROR(logger, "send error: {}", ec.message());
        // send client that this connection was disconnected
        net::pack response;
        response.set_id(pack.id());
        response.set_type(net::pack::Type::pack_Type_disconnect);

        co_await RequireSendToClient(response);

        // remove connection
        LOG_INFO(logger, "remove connection id:{}", pack.id());
        ul.lock();
        connection.erase(it);
    }

}

TunnelListener::TunnelListener(uint16_t port, std::function<awaitable<void>(uint64_t)> NewConnection,
                               std::function<awaitable<void>(const net::pack &)> SendToClient,
                               std::function<void()> RequireDestroy) :
        port(port),
        RequireNewConnection(std::move(NewConnection)),
        RequireSendToClient(std::move(SendToClient)),
        RequireDestroy(std::move(RequireDestroy))
{
    TRACE_FUNC(logger);
}

TunnelListener::~TunnelListener() {
    TRACE_FUNC(logger);
    Pool<tcp::acceptor>::GetInstance().ReleaseAcceptor(port);
}

awaitable<void> TunnelListener::ResponseNewConnection(net::pack response) {
    // check if response port is in waitAckConnection or connection
    std::unique_lock ul1{waitAckConnectionMutex, std::defer_lock};
    std::unique_lock ul2{connectionMutex, std::defer_lock};
    std::lock(ul1, ul2);

    if (!waitAckConnection.contains(response.id()) && !connection.contains(response.id())) {
        LOG_ERROR(logger, "id:{} not found", response.id());
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
        LOG_ERROR(logger, "id:{} already exist in connection", response.id());
        // unexplained circumstances
        co_return ;
    }


    // move socket from waitAckConnection to connection
    connection.insert(waitAckConnection.extract(response.id()));
    ul1.unlock();
    ul2.unlock();

    co_spawn(co_await this_coro::executor, RecvFromConnection(response.id()), asio::detached);

    LOG_INFO(logger, "add connection id:{}", response.id());
}

awaitable<void> TunnelListener::RecvFromConnection(uint64_t id) {
    TRACE_FUNC(logger);
    std::unique_lock ul{connectionMutex};
    auto &conn{connection.at(id)};
    ul.unlock();
    std::vector<char> buffer(1024 * 1024 * 2);
    for(;;){
        auto [ec, size] = co_await conn.async_read_some(asio::buffer(buffer), use_nothrow_awaitable);
        if (ec){
            LOG_ERROR(logger, "recv error: {}", ec.message());
            // send client that this connection was disconnected
            net::pack pack;
            pack.set_id(id);
            pack.set_type(net::pack::disconnect);
            pack.set_port(port);
            co_await RequireSendToClient(pack);
            ul.lock();
            connection.erase(id);
            co_return;
        }
        LOG_TRACE(logger, "recv {} bytes from id:{}", size, id);

        net::pack pack;
        pack.set_id(id);
        pack.set_type(net::pack::translate);
        pack.set_port(port);
        pack.set_data({buffer.data(), buffer.data() + size});

        co_await RequireSendToClient(pack);
    }
}

awaitable<void> TunnelListener::operator()() {
    TRACE_FUNC(logger);
    auto acceptor = AcceptorPool::GetInstance().GetAcceptor(this->port);
    if (!acceptor){
        LOG_ERROR(logger, "Get acceptor(from port {}) failed)", this->port);
        co_return ;
    }
    for(;;) {
        auto [ec, socket] = co_await acceptor->async_accept(use_nothrow_awaitable);
        if (ec) {
            LOG_ERROR(logger, "accept error: {}", ec.message());
            this->RequireDestroy();
            co_return;
        }
        LOG_INFO(logger, "recv new connection");
        auto this_cnt = cnt++;
        std::unique_lock ul{waitAckConnectionMutex};
        this->waitAckConnection.emplace(this_cnt, std::move(socket));
        ul.unlock();

        co_spawn(co_await this_coro::executor, this->RequireNewConnection(this_cnt), asio::detached);
        co_spawn(co_await this_coro::executor, [](auto this_cnt, auto self) -> awaitable<void> {
            co_await timeout(5s);
            std::lock_guard lg{self->waitAckConnectionMutex};
            auto success = self->waitAckConnection.erase(this_cnt);
            if (success > 0)
                LOG_ERROR(self->logger, "id:{} timeout", this_cnt);
        }(this_cnt, this), asio::detached);
    }

}

awaitable<void> TunnelListener::Destroy() {
    TRACE_FUNC(logger);
    std::unique_lock ul(connectionMutex, std::defer_lock);
    std::unique_lock ul2(waitAckConnectionMutex, std::defer_lock);
    std::lock(ul, ul2);
    for (auto &[_, socket] : connection)
        socket.close();
    for (auto &[_, socket] : waitAckConnection)
        socket.close();
    ul2.unlock();
    ul.unlock();


    co_await timeout(1s);
    co_return;
}


