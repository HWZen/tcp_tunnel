//
// Created by HWZ on 2023/6/23.
//

#ifndef TCP_TUNNEL_CLIENTPROCESS_H
#define TCP_TUNNEL_CLIENTPROCESS_H
#include "asio_pch.h"
#include <unordered_set>
#include "proto/net_pack.pb.h"

class ClientProcess {
public:
    tcp::socket socket;
    std::unordered_set<uint16_t> usedPort{};

    awaitable<void> operator()();

    awaitable<void> ProcessRequest(const net::listen_request &request);

    awaitable<void> ProcessPackage(const net::pack &pack);

};


#endif //TCP_TUNNEL_CLIENTPROCESS_H
