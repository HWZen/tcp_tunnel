//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#ifndef TCP_TUNNEL_GATEWAY_H
#define TCP_TUNNEL_GATEWAY_H
#include "asio_pch.h"
#include "TunnelListener.h"
#include <unordered_set>
#include <memory>

class Gateway
{
public:

    explicit Gateway(
            uint16_t port
            );

    asio::io_context ioContext;
    std::shared_ptr<tcp::acceptor> listenAcceptor;


    awaitable<void> Accept();

    awaitable<void> CreateClientProcessor(tcp::socket socket);

    size_t spin();

};


#endif //TCP_TUNNEL_GATEWAY_H
