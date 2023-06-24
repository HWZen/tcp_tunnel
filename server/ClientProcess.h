//
// Created by HWZ on 2023/6/23.
//

#ifndef TCP_TUNNEL_CLIENTPROCESS_H
#define TCP_TUNNEL_CLIENTPROCESS_H
#include "asio_pch.h"
#include "proto/net_pack.pb.h"
#include "server/TunnelListener.h"
#include <unordered_set>

class ClientProcess {
public:
    tcp::socket socket;
    std::unordered_map<uint16_t, std::unique_ptr<TunnelListener>> usedTunnels{};

    awaitable<void> operator()();

    awaitable<void> ProcessRequest(const net::listen_request &request);

    awaitable<void> ProcessPackage(const net::pack &pack);

};


#endif //TCP_TUNNEL_CLIENTPROCESS_H
