//
// Created by HWZ on 2023/6/23.
//

#include "ClientProcess.h"
#include "Pool.h"
using AcceptorPool = Pool<tcp::acceptor>;

// TODO: One client should only have one tunnel
awaitable<void> ClientProcess::operator()() {
    TRACE_FUNC(logger);
    for(;;){
        auto [ec, buffer] = co_await RecvMsg(socket);
        LOG_TRACE(logger, "recv {} bytes", buffer.size());
        if (ec){
            LOG_ERROR(logger, "recv error: {}", ec.message());
            break;
        }

        net::data data;
        if (!data.ParseFromArray(buffer.data(), static_cast<int>(buffer.size()))){
            LOG_ERROR(logger, "parse data error");
            continue;
        }

        if (data.has_listen_request()) {
            co_spawn(co_await this_coro::executor, ProcessRequest(data.listen_request()), asio::detached);
        }
        else if (data.has_pack()){
            co_spawn(co_await this_coro::executor, ProcessPackage(data.pack()), asio::detached);
        }
        else{
            LOG_WARN(logger, "unknown data type");
        }
    }

    // close all acceptors
    for (auto& [port, _] : usedTunnels){
        auto acceptor = AcceptorPool::GetInstance().GetAcceptor(port);
        if (acceptor)
            acceptor->close();
    }
    using namespace std::chrono_literals;
    co_await timeout(1s);

    LOG_INFO(logger, "ClientProcess() exit");
}

awaitable<void> ClientProcess::ProcessRequest(net::listen_request request) try {
    // Check if port is used
    TRACE_FUNC(logger);
    LOG_INFO(logger, "requesting port:{}", request.port());
    if (usedTunnels.find(static_cast<uint16_t>(request.port())) != usedTunnels.end()
        || AcceptorPool::GetInstance().HasAcceptor(static_cast<uint16_t>(request.port())))
    {
        LOG_ERROR(logger, "port {} is used", request.port());
        net::data responseData;
        auto response{responseData.mutable_listen_response()};
        response->set_port(request.port());
        response->set_status(net::listen_response::other_host_listening);

        auto [ec, _] = co_await SendMsg(socket, responseData);
        if (ec) {
            LOG_ERROR(logger, "send error: {}", ec.message());
        }

        co_return;

    }

    // Create acceptor
    auto acceptor{std::make_shared<tcp::acceptor>(co_await this_coro::executor, tcp::endpoint(tcp::v4(),
                                                                                              static_cast<asio::ip::port_type>(request.port())))};

    if(!AcceptorPool::GetInstance().AddAcceptor(static_cast<uint16_t>(request.port()), std::move(acceptor))){
        // log it
        LOG_ERROR(logger, "add acceptor error");
        net::data responseData;
        auto response{responseData.mutable_listen_response()};
        response->set_port(request.port());
        response->set_status(net::listen_response::other_host_listening);

        auto [ec, _] = co_await SendMsg(socket, responseData);
        if (ec) {
            LOG_ERROR(logger, "send error: {}", ec.message());
            co_return;
        }
    }


    // Insert new TunnelListener

    auto RequireNewConnectionFunc = [
            port = request.port(),
            &socket = this->socket,
            &logger = this->logger
            ](uint64_t id) -> awaitable<void>{
        TRACE_FUNC(logger);
        // Send to client
        net::data data;
        auto& pack{*data.mutable_pack()};
        pack.set_id(id);
        pack.set_port(port);
        pack.set_type(net::pack::connect);


        auto [ec, _] = co_await SendMsg(socket, data);
        if (ec){
            LOG_ERROR(logger, "send error: {}", ec.message());
        }
        // client will respond
    };

    auto RequireSendToClientFunc = [
            &socket = this->socket,
            &logger = this->logger
            ](const net::pack &pack) -> awaitable<void>{
        TRACE_FUNC(logger);
        net::data data;
        *data.mutable_pack() = pack;

        auto [ec, _] = co_await SendMsg(socket, data);
        if (ec){
            LOG_ERROR(logger, "send error: {}", ec.message());
        }

    };

    auto RequireDestroyFunc = [
            this,
            port = request.port()
            ]() -> void{
        TRACE_FUNC(logger);
        usedTunnels.erase(static_cast<uint16_t>(port));
    };

    auto tunnel = std::make_shared<TunnelListener>(static_cast<uint16_t>(request.port()),
                                                   RequireNewConnectionFunc,
                                                   RequireSendToClientFunc,
                                                   RequireDestroyFunc
                                                   );

    usedTunnels.emplace(request.port(), tunnel);

    co_spawn(co_await this_coro::executor, [](auto tunnel)->awaitable<void>{
        co_await (*tunnel)();
        co_await tunnel->Destroy();
    }(std::move(tunnel)), asio::detached);

    LOG_INFO(logger,"Listen new port: {}", request.port());

    // send response
    net::data responseData;
    auto& response{*responseData.mutable_listen_response()};
    response.set_port(request.port());
    response.set_status(net::listen_response::success);

    auto [ec, _] = co_await SendMsg(socket, responseData);
    if (ec){
        LOG_ERROR(logger, "send error: {}", ec.message());
        usedTunnels.erase(static_cast<uint16_t>(request.port()));
    }

    co_return;
}
catch(std::exception& e){
    LOG_CRITICAL(logger, "catch exception: {}", e.what());
    co_return;
}

awaitable<void> ClientProcess::ProcessPackage(net::pack pack) {
    TRACE_FUNC(logger);
    // check pack illegal
    if (!usedTunnels.contains(static_cast<uint16_t>(pack.port()))){
        LOG_ERROR(logger, "port {} not exist", pack.port());
        // send to client that server fail
        net::data data;
        auto &response{*data.mutable_listen_response()};
        response.set_port(pack.port());
        response.set_status(net::listen_response::listen_fail);

        auto [ec,_] = co_await SendMsg(socket, data);
        if (ec){
            LOG_ERROR(logger, "send error: {}", ec.message());
        }
        co_return;
    }

    // send to tunnel
    auto p_tunnel{usedTunnels.at(static_cast<uint16_t>(pack.port())).lock()};
    if (!p_tunnel)
    {
        LOG_ERROR(logger, "port {} not exist", pack.port());
        // send to client that server fail
        net::data data;
        auto &response{*data.mutable_listen_response()};
        response.set_port(pack.port());
        response.set_status(net::listen_response::listen_fail);

        auto [ec,_] = co_await SendMsg(socket, data);
        if (ec)
            LOG_ERROR(logger, "send error: {}", ec.message());

        usedTunnels.erase(static_cast<uint16_t>(pack.port()));
        co_return;
    }
    auto &tunnel{*p_tunnel};
    co_await tunnel.ProcessPack(pack);

    co_return;
}
