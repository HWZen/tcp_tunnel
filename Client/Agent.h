//
// Created by HWZ on 2023/6/26.
//

#ifndef TCP_TUNNEL_AGENT_H
#define TCP_TUNNEL_AGENT_H
#include "asio_pch.h"
#include "Advertiser.h"
#include <string_view>
#include "Logger.h"

class Agent {
public:
    Agent(
            std::string_view proxyAddress,
            uint16_t proxyPort,
            std::string_view serverAddress,
            uint16_t serverPort,
            uint16_t requestPort
            );

    void Connect();

    void Run();

private:
    awaitable<void> RecvFromConnection(uint64_t id);

private:
    std::string proxyAddress;
    uint16_t proxyPort;
    std::string serverAddress;
    uint16_t serverPort;
    uint16_t requestPort;

    asio::io_context ioContext;
    Advertiser advertiser;

    std::unordered_map<uint64_t, tcp::socket> connections;

    awaitable<bool> NewConnection(uint64_t id);
    awaitable<void> DisConnection(uint64_t id);
    awaitable<void> RecvData(uint64_t id, std::string recvData);

    Logger logger{"Agent"};
};


#endif //TCP_TUNNEL_AGENT_H
