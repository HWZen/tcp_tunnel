//
// Created by HWZ on 2023/6/24.
//

#ifndef TCP_TUNNEL_ADVERTISER_H
#define TCP_TUNNEL_ADVERTISER_H
#include "asio_pch.h"
#include "proto/net_pack.pb.h"
#include "Logger.h"
#include <cstdint>
#include <string_view>
#include <functional>


class Advertiser {

public:
    Advertiser(asio::io_context &executor,
               std::function<awaitable<bool>(uint64_t)> NewConnection,
               std::function<awaitable<void>(uint64_t)> DisConnection,
               std::function<awaitable<void>(uint64_t, std::string)> RecvData);

    asio::error_code connect(std::string_view ServerIp, uint16_t port, uint16_t proxyPort);

    awaitable<asio::error_code> AsyncConnect(std::string ServerIp, uint16_t port, uint16_t proxyPort);

    void AsyncConnect(std::string_view ServerIp, uint16_t port, uint16_t proxyPort,
                      std::function<void(std::exception_ptr e, asio::error_code)> callback);

    awaitable<void> DisConnection(uint64_t id);

    awaitable<void> SendToServer(uint64_t id, std::string_view data);


private:
    awaitable<void> RecvFromServer();

    awaitable<void> HeartBeat();

private:
    tcp::socket server;

    uint16_t proxyPort{};

    std::function<awaitable<bool>(uint64_t)> ReqNewConnection;
    std::function<awaitable<void>(uint64_t)> ReqDisConnection;
    std::function<awaitable<void>(uint64_t, std::string)> RecvData;

    volatile bool heartbeatFlag{false};

    Logger logger{"Advertiser"};
};


#endif //TCP_TUNNEL_ADVERTISER_H
