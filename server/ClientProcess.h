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
    ClientProcess(tcp::socket socket) : socket(std::move(socket)) {}
    awaitable<void> operator()();

    awaitable<void> ProcessRequest(net::listen_request request);

    awaitable<void> ProcessPackage(net::pack pack);

private:
    tcp::socket socket;
    std::unordered_map<uint16_t, std::unique_ptr<TunnelListener>> usedTunnels{};
    Logger logger{"ClientProcess"};
};


#endif //TCP_TUNNEL_CLIENTPROCESS_H
