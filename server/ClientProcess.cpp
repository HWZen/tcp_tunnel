//
// Created by HWZ on 2023/6/23.
//

#include "ClientProcess.h"
#include "Pool.h"
using AcceptorPool = Pool<tcp::acceptor>;

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
    // Check if port is used
    if (usedTunnels.find(request.port()) != usedTunnels.end()
        || AcceptorPool::GetInstance().HasAcceptor(request.port()))
    {
        // log it
        net::data responseData;
        auto response{responseData.mutable_listen_response()};
        response->set_port(request.port());
        response->set_status(net::listen_response::other_host_listening);

        uint64_t sendLen{responseData.ByteSizeLong()};
        std::array<asio::const_buffer, 2> bufferSequence{
            asio::buffer(&sendLen, sizeof(sendLen)),
            asio::buffer(responseData.SerializeAsString())
        };
        auto [ec, _] = co_await socket.async_write_some(bufferSequence, use_nothrow_awaitable);
        if (ec) {
            // log it
            co_return;
        }


    }

    // Create acceptor
    auto acceptor{std::make_shared<tcp::acceptor>(co_await this_coro::executor, tcp::endpoint(tcp::v4(), request.port()))};
    if(!AcceptorPool::GetInstance().AddAcceptor(request.port(), std::move(acceptor))){
        // log it
        net::data responseData;
        auto response{responseData.mutable_listen_response()};
        response->set_port(request.port());
        response->set_status(net::listen_response::other_host_listening);

        uint64_t sendLen{responseData.ByteSizeLong()};
        std::array<asio::const_buffer, 2> bufferSequence{
            asio::buffer(&sendLen, sizeof(sendLen)),
            asio::buffer(responseData.SerializeAsString())
        };
        auto [ec, _] = co_await socket.async_write_some(bufferSequence, use_nothrow_awaitable);
        if (ec) {
            // log it
            co_return;
        }
    }


    // Insert new TunnelListener

    auto RequireNewConnectionFunc = [
            port = request.port(),
            &socket = this->socket
            ](uint64_t id) -> awaitable<void>{
        // Send to client
        net::data data;
        auto& pack{*data.mutable_pack()};
        pack.set_id(id);
        pack.set_port(port);
        pack.set_type(net::pack::connect);

        uint64_t len{data.ByteSizeLong()};
        std::array<asio::const_buffer, 2> bufferSequence{
            asio::buffer(&len, sizeof(len)),
            asio::buffer(data.SerializeAsString())
        };

        auto [ec, _] = co_await socket.async_write_some(bufferSequence, use_nothrow_awaitable);
        if (ec){
            // log it
        }

        // client will respond
    };

    auto RequireSendToClientFunc = [
            &socket = this->socket
            ](const net::pack &pack) -> awaitable<void>{
        net::data data;
        *data.mutable_pack() = pack;

        uint64_t len{data.ByteSizeLong()};
        std::array<asio::const_buffer, 2> bufferSequence{
            asio::buffer(&len, sizeof(len)),
            asio::buffer(data.SerializeAsString())
        };

        auto [ec, _] = co_await socket.async_write_some(bufferSequence, use_nothrow_awaitable);
        if (ec){
            // log it
        }

    };

    auto RequireDestroyFunc = [
            this,
            port = request.port()
            ]() -> void{
        usedTunnels.erase(port);
    };

    auto tunnel = std::make_unique<TunnelListener>(co_await this_coro::executor,
                                                   static_cast<uint16_t>(request.port()),
                                                   RequireNewConnectionFunc,
                                                   RequireSendToClientFunc,
                                                   RequireDestroyFunc
                                                   );

    usedTunnels.emplace(request.port(), std::move(tunnel));
    co_return;
}

awaitable<void> ClientProcess::ProcessPackage(const net::pack &pack) {
    // check pack illegal
    if (!usedTunnels.contains(pack.port())){
        // log it
        // send to client that server fail
        net::data data;
        auto &response{*data.mutable_listen_response()};
        response.set_port(pack.port());
        response.set_status(net::listen_response::listen_fail);

        uint64_t len{data.ByteSizeLong()};
        std::array<asio::const_buffer, 2> bufferSequence{
                asio::buffer(&len, sizeof(len)),
                asio::buffer(data.SerializeAsString())
        };

        auto [ec,_] = co_await socket.async_write_some(bufferSequence, use_nothrow_awaitable);
        if (ec){
            // log it
        }
        co_return;
    }

    // send to tunnel
    auto &tunnel{*usedTunnels.at(pack.port())};
    co_await tunnel.SendToConnection(pack);


    co_return;
}
