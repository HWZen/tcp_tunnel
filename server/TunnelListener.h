//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#ifndef TCP_TUNNEL_TUNNELLISTENER_H
#define TCP_TUNNEL_TUNNELLISTENER_H
#include "proto/net_pack.pb.h"
#include "asio_pch.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>



class TunnelListener {
public:
    TunnelListener(asio::io_context& io_context,
            std::shared_ptr<tcp::acceptor> acceptor,
            std::function<awaitable<void>(uint64_t)> NewConnection,
            std::function<awaitable<void>(const net::pack &)> SendToClient,
            std::function<void()> RequireDestroy
            );

    std::shared_ptr<tcp::acceptor> acceptor;
    std::atomic_int64_t cnt{0};

    std::mutex connectionMutex;
    std::unordered_map<uint64_t, tcp::socket> connection;
    std::mutex waitAckConnectionMutex;
    std::unordered_map<uint64_t, tcp::socket> waitAckConnection;

    std::function<awaitable<void>(uint64_t)> RequireNewConnection;

    awaitable<void> ClientCallback(const net::pack& pack);

    awaitable<void> SendToConnection(const net::pack& pack);

    std::function<awaitable<void>(const net::pack &)> RequireSendToClient;

    std::function<void()> RequireDestroy;

    ~TunnelListener();

};


#endif //TCP_TUNNEL_TUNNELLISTENER_H
