//
// Created by HWZ on 2023/6/26.
//

#include "Agent.h"

Agent::Agent(
        std::string_view proxyAddress,
        uint16_t proxyPort,
        std::string_view serverAddress,
        uint16_t serverPort,
        uint16_t requestPort):
        proxyAddress(proxyAddress),
        proxyPort(proxyPort),
        serverAddress(serverAddress),
        serverPort(serverPort),
        requestPort(requestPort),
        advertiser(ioContext,
                   [this](uint64_t id) -> awaitable<bool> { co_return co_await this->NewConnection(id); },
                   [this](uint64_t id) -> awaitable<void> { co_await this->DisConnection(id); },
                   [this](uint64_t id, std::string buf) -> awaitable<void> { co_await this->RecvData(id, buf); })

{
    TRACE_FUNC(logger);

    // init heartBeatTimer
    heartBeatTimer = std::make_unique<asio::steady_timer>(ioContext);

    LOG_INFO(logger, "proxyAddress: {}, proxyPort: {}, serverAddress: {}, serverPort: {}, requestPort: {}",
             proxyAddress, proxyPort, serverAddress, serverPort, requestPort);
}

void Agent::Connect() {
    TRACE_FUNC(logger);
    auto ec{advertiser.connect(serverAddress, serverPort, requestPort)};
    if (ec){
        LOG_ERROR(logger, "connect to server failed: {}", ec.message());
        throw std::logic_error(ec.message());
    }
}

void Agent::Run() {
    TRACE_FUNC(logger);
    ioContext.run();
    heartBeatTimer.reset(nullptr);
}

awaitable<bool> Agent::NewConnection(uint64_t id) {
    TRACE_FUNC(logger);
    auto& connection{connections.emplace(id, ioContext).first->second};
    auto [ec] = co_await connection.async_connect(tcp::endpoint{asio::ip::address::from_string(proxyAddress), proxyPort}, use_nothrow_awaitable);
    LOG_DEBUG(logger, "connected to native server");

    if (ec){
        LOG_ERROR(logger, "connect to proxy failed: {}", ec.message());
        connections.erase(id);
        co_return false;
    }

    LOG_INFO(logger, "new connection: {}", id);

    co_spawn(ioContext, RecvFromConnection(id), asio::detached);

    co_return true;
}

awaitable<void> Agent::DisConnection(uint64_t id) {
    TRACE_FUNC(logger);
    auto success = connections.erase(id);
    LOG_INFO(logger, "erase connection {} : {}", id, success == 0 ? "not exist": "success");
    co_return;
}

awaitable<void> Agent::RecvData(uint64_t id, std::string recvData) {
    TRACE_FUNC(logger);
    if (!connections.contains(id)){
        LOG_ERROR(logger, "connection {} not exist", id);
        co_await advertiser.DisConnection(id);
        co_return ;
    }
    LOG_TRACE(logger, "recv data from connection {}, size: {}", id, recvData.size());

    auto& connection{connections.at(id)};
    auto [ec, len] = co_await connection.async_write_some(asio::buffer(recvData), use_nothrow_awaitable);
    if (ec){
        LOG_ERROR(logger, "write to connection {} failed: {}", id, ec.message());
        connections.erase(id);
        co_await advertiser.DisConnection(id);
    }
}

awaitable<void> Agent::RecvFromConnection(uint64_t id) {
    TRACE_FUNC(logger);
    auto& connection{connections.at(id)};
    std::vector<char> buffer(1024 * 1024 * 2);
    for(;;){
        auto [ec, len] = co_await connection.async_read_some(asio::buffer(buffer), use_nothrow_awaitable);
        if (ec){
            LOG_ERROR(logger, "read from connection {} failed: {}", id, ec.message());
            co_await advertiser.DisConnection(id);
            connections.erase(id);
            co_return;
        }

        LOG_TRACE(logger, "read from connection {}, size: {}", id, len);
        co_await advertiser.SendToServer(id, std::string_view{buffer.data(), len});
    }
}
