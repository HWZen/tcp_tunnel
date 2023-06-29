//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#ifndef TCP_TUNNEL_TUNNELLISTENER_H
#define TCP_TUNNEL_TUNNELLISTENER_H
#include "proto/net_pack.pb.h"
#include "asio_pch.h"
#include "Logger.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>



class TunnelListener {
public:
    TunnelListener(const asio::any_io_executor &io_context,
            uint16_t port,
            std::function<awaitable<void>(uint64_t)> NewConnection,
            std::function<awaitable<void>(const net::pack &)> SendToClient,
            std::function<void()> RequireDestroy
            );



    awaitable<void> ProcessPack(const net::pack& pack);

    ~TunnelListener();
private:
    awaitable<void> SendToConnection(net::pack pack);

    awaitable<void> ResponseNewConnection(net::pack response);
    awaitable<void> RecvFromConnection(uint64_t id);

private:
    uint16_t port;
    std::atomic_int64_t cnt{0};

    std::mutex connectionMutex;
    std::unordered_map<uint64_t, tcp::socket> connection;

    std::mutex waitAckConnectionMutex;
    std::unordered_map<uint64_t, tcp::socket> waitAckConnection;

    std::function<awaitable<void>(uint64_t)> RequireNewConnection{};
    std::function<awaitable<void>(const net::pack &)> RequireSendToClient{};
    std::function<void()> RequireDestroy;

    Logger logger{"TunnelListener"};
};


#endif //TCP_TUNNEL_TUNNELLISTENER_H
