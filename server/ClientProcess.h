//
// Created by HWZ on 2023/6/23.
//

#ifndef TCP_TUNNEL_CLIENTPROCESS_H
#define TCP_TUNNEL_CLIENTPROCESS_H
#include "asio_pch.h"
#include "proto/net_pack.pb.h"
#include "server/TunnelListener.h"
#include "Logger.h"
#include <unordered_set>

class ClientProcess {
public:
    explicit ClientProcess(tcp::socket socket) : socket(std::move(socket)), heartBeatTimer(this->socket.get_executor()) {
        co_spawn(this->socket.get_executor(), HeartBeat(), asio::detached);
    }
    awaitable<void> operator()();

    awaitable<void> ProcessRequest(net::listen_request request);

    awaitable<void> ProcessPackage(net::pack pack);

private:
    awaitable<void> HeartBeat();
private:
    tcp::socket socket;
    std::unordered_map<uint16_t, std::weak_ptr<TunnelListener>> usedTunnels{};

    asio::steady_timer heartBeatTimer;
//    volatile bool heartbeatFlag{false};

    Logger logger{"ClientProcess"};
};


#endif //TCP_TUNNEL_CLIENTPROCESS_H
