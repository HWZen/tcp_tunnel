//
// Created by HWZ on 2023/6/24.
//

#include "Advertiser.h"


Advertiser::Advertiser(asio::io_context &executor,
                       std::function<awaitable<bool>(uint64_t)> NewConnection,
                       std::function<awaitable<void>(uint64_t)> DisConnection,
                       std::function<awaitable<void>(uint64_t, std::string)> RecvData) :
        server(executor),
        ReqNewConnection(std::move(NewConnection)),
        ReqDisConnection(std::move(DisConnection)),
        RecvData(std::move(RecvData))
{
    TRACE_FUNC(logger);
}

asio::error_code Advertiser::connect(std::string_view ServerIp, uint16_t port, uint16_t proxyPort) {
    TRACE_FUNC(logger);
    LOG_INFO(logger, "try to connect to server: {}:{}, proxy port: {}", ServerIp, port, proxyPort);
    // connect to server
    asio::error_code ec;
    server.connect({asio::ip::make_address(ServerIp), port}, ec);
    if (ec){
        LOG_ERROR(logger, "connect to server failed, ec: {}", ec.message());
        return ec;
    }

    // advertise the port listened request
    net::data data;
    auto& listenRequest{*data.mutable_listen_request()};
    listenRequest.set_port(proxyPort);

    // send data
    auto bufferSeq{MakeSendSeq(data)};
    bufferSeq.ref();
    server.write_some(bufferSeq, ec);
    if (ec){
        LOG_ERROR(logger, "send listen request failed, ec: {}", ec.message());
        return ec;
    }

    // wait for server response
    uint64_t len;
    server.read_some(asio::buffer(&len, sizeof(len)), ec);
    if (ec){
        LOG_ERROR(logger, "recv listen response failed, ec: {}", ec.message());
        return ec;
    }
    std::vector<char> recvBuf(static_cast<size_t>(len));
    server.read_some(asio::buffer(recvBuf), ec);
    if (ec){
        LOG_ERROR(logger, "recv listen response failed, ec: {}", ec.message());
        return ec;
    }

    // parse response
    net::data responseData;
    if (!responseData.ParseFromArray(recvBuf.data(), static_cast<int>(recvBuf.size()))){
        LOG_ERROR(logger, "parse listen response failed");
        return std::make_error_code(std::errc::bad_message);
    }

    // check response
    if (!responseData.has_listen_response()){
        LOG_ERROR(logger, "listen response is not found");
        return std::make_error_code(std::errc::bad_message);
    }

    const auto& response{responseData.listen_response()};
    if (response.status() != net::listen_response::success){
        LOG_ERROR(logger, "listen response status is not success, status: {}", response.status());
        return std::make_error_code(std::errc::connection_refused);
    }

    LOG_INFO(logger, "connect to server success, proxy port: {}", proxyPort);

    // start listening
    co_spawn(server.get_executor(), RecvFromServer(), asio::detached);
    // start heart beat
    co_spawn(server.get_executor(), HeartBeat(), asio::detached);

    this->proxyPort = proxyPort;
    return {};
}

awaitable<asio::error_code> Advertiser::AsyncConnect(std::string ServerIp, uint16_t port, uint16_t proxyPort) {
    TRACE_FUNC(logger);
    // connect to server
    auto [ec] = co_await server.async_connect({asio::ip::make_address(ServerIp), port}, use_nothrow_awaitable);
    if (ec){
        LOG_ERROR(logger, "connect to server failed, ec: {}", ec.message());
        co_return ec;
    }

    // advertise the port listened request
    net::data data;
    auto& listenRequest{*data.mutable_listen_request()};
    listenRequest.set_port(proxyPort);

    // send data
    auto [ec2, _] = co_await SendMsg(server, data);
    if (ec2){
        LOG_ERROR(logger, "send listen request failed, ec: {}", ec2.message());
        co_return ec2;
    }

    // wait for server response
    auto [ec3, recvBuf] = co_await RecvMsg(server);
    if (ec3){
        LOG_ERROR(logger, "recv listen response failed, ec: {}", ec3.message());
        co_return ec3;
    }

    // parse response
    net::data responseData;
    if (!responseData.ParseFromArray(recvBuf.data(), static_cast<int>(recvBuf.size()))){
        LOG_ERROR(logger, "parse listen response failed");
        co_return std::make_error_code(std::errc::bad_message);
    }

    // check response
    if (!responseData.has_listen_response()){
        LOG_ERROR(logger, "listen response is not found");
        co_return std::make_error_code(std::errc::bad_message);
    }

    const auto& response{responseData.listen_response()};
    if (response.status() != net::listen_response::success){
        LOG_ERROR(logger, "listen response status is not success, status: {}", response.status());
        co_return std::make_error_code(std::errc::connection_refused);
    }

    LOG_INFO(logger, "connect to server success, proxy port: {}", proxyPort);

    // start listening
    co_spawn(co_await this_coro::executor, RecvFromServer(), asio::detached);
    // start heart beat
    co_spawn(co_await this_coro::executor, HeartBeat(), asio::detached);
    this->proxyPort = proxyPort;
    co_return asio::error_code{};
}

void Advertiser::AsyncConnect(std::string_view ServerIp, uint16_t port, uint16_t proxyPort,
                              std::function<void(std::exception_ptr e, asio::error_code)> callback) {
    TRACE_FUNC(logger);
    co_spawn(server.get_executor(), AsyncConnect({ServerIp.data(), ServerIp.size()}, port, proxyPort), callback);
}

awaitable<void> Advertiser::RecvFromServer() {
    TRACE_FUNC(logger);
    for (;;){
        auto [ec, recvBuf] = co_await RecvMsg(server);
        if (ec){
            LOG_ERROR(logger, "recv from server failed, ec: {}", ec.message());
            break;
        }

        // parse data
        net::data data;
        if (!data.ParseFromArray(recvBuf.data(), static_cast<int>(recvBuf.size()))){
            LOG_ERROR(logger, "parse data failed");
            continue;
            // TODO: Can not continue, because we don't know where a pack start, how should fix it?
        }

        if (data.has_listen_response() && data.listen_response().status() != net::listen_response::success){
            // server has some problem, so we should cancel this listening
            LOG_WARN(logger, "server send listen response failed, status: {}", data.listen_response().status());
            LOG_WARN(logger, "server has some problem, cancel this listening");
            break;
        }

        if (!data.has_pack()){
            LOG_WARN(logger, "pack is not found");
            continue;
        }

        const auto& pack{data.pack()};

        // check pack
        switch (pack.type()){
            case net::pack::connect:
                LOG_INFO(logger, "new connection, id: {}", pack.id());
                co_spawn(co_await this_coro::executor, [&ReqNewConnection = this->ReqNewConnection,
                                                        id = pack.id(),
                                                        proxyPort = this->proxyPort,
                                                        &server = this->server,
                                                        &logger = this->logger] () -> awaitable<void>
                    {
                        auto res = co_await ReqNewConnection(id);
                        net::data data;
                        auto& pack{*data.mutable_pack()};
                        pack.set_id(id);
                        pack.set_port(proxyPort);
                        if (res)
                            pack.set_type(net::pack::connect);
                        else
                            pack.set_type(net::pack::disconnect);

                        auto [ec2, _] = co_await SendMsg(server, data);

                        if (ec2){
                            LOG_ERROR(logger, "send server response failed, ec: {}", ec2.message());
                        }

                    }, asio::detached);
                break;
            case net::pack::disconnect:
                LOG_INFO(logger, "disconnection, id: {}", pack.id());
                co_spawn(co_await this_coro::executor, ReqDisConnection(pack.id()), asio::detached);
                break;
            case net::pack::translate:
                LOG_TRACE(logger, "translate data, id: {}", pack.id());
                co_spawn(co_await this_coro::executor, RecvData(pack.id(), pack.data()), asio::detached);
                break;
            case net::pack::pong:
                LOG_TRACE(logger, "recv pong, id: {}", pack.id());
                break;
            default:
                LOG_WARN(logger, "unknown pack type: {}", pack.type());
                break;
        }
    }

    // exit
    server.close();
    heartBeatTimer->cancel();

    co_return;
}

awaitable<void> Advertiser::DisConnection(uint64_t id) {
    TRACE_FUNC(logger);
    net::data data;
    auto& pack{*data.mutable_pack()};
    pack.set_id(id);
    pack.set_port(proxyPort);
    pack.set_type(net::pack::disconnect);
    co_await SendMsg(server, data);
}

awaitable<void> Advertiser::SendToServer(uint64_t id, std::string_view buffer) {
    TRACE_FUNC(logger);

    net::data data;
    auto& pack{*data.mutable_pack()};
    pack.set_id(id);
    pack.set_type(net::pack::translate);
    pack.set_port(proxyPort);
    *pack.mutable_data() = buffer;


    LOG_TRACE(logger, "send to server, id: {}, len: {}", id, pack.ByteSizeLong());

    auto [ec, _] = co_await SendMsg(server, data);
    if (ec){
        LOG_ERROR(logger, "send to server failed, ec: {}", ec.message());
    }



}

awaitable<void> Advertiser::HeartBeat() {
    TRACE_FUNC(logger);
    LOG_DEBUG(logger,"in heard beat");
    using namespace std::chrono_literals;
    for (;;){
        if (!heartBeatTimer){
            LOG_WARN(logger, "heart beat timer not found");
            co_return;
        }
        heartBeatTimer->expires_after(1min);
        auto [ec] = co_await heartBeatTimer->async_wait(use_nothrow_awaitable);
        if (!server.is_open())
            co_return;
        if (ec){
            if (heartbeatFlag)
                heartbeatFlag =  false;
            continue; // normal, heart beat be canceled
        }

        // timeout
        if (heartbeatFlag){
            // recv response timeout
            LOG_ERROR(logger, "recv heart beat response timeout");
            server.close();
            co_return;
        }

        // timeout, send heart beat
        LOG_TRACE(logger, "Timeout, send heart beat");
        net::data data;
        auto& pack{*data.mutable_pack()};
        pack.set_type(net::pack::ping);
        pack.set_port(proxyPort);
        auto [ ec2, _] = co_await SendMsg(server, data);
        if (ec2){
            LOG_ERROR(logger, "send heartbeat failed, ec: {}", ec.message());
            server.close();
            co_return;
        }
        heartbeatFlag = true;
    }
}
