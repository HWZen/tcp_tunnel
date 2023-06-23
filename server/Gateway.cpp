//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#include "Gateway.h"
#include "Pool.h"
#include "ClientProcess.h"


using AcceptorPool = Pool<tcp::acceptor>;

Gateway::Gateway(uint16_t port)
{
    if (AcceptorPool::GetInstance().HasAcceptor(port)){
        // log it
        throw std::runtime_error("port already in use");
    }
    listenAcceptor = std::make_shared<tcp::acceptor>(ioContext, tcp::endpoint(tcp::v4(), port));
    co_spawn(ioContext, Accept(), asio::detached);
}

awaitable<void> Gateway::Accept() {
    for(;;){
        auto [ec, socket] = co_await listenAcceptor->async_accept(use_nothrow_awaitable);
        if (ec) {
            // log it
            break;
        }

        co_spawn(ioContext, CreateClientProcessor(std::move(socket)), asio::detached);
    }
}

awaitable<void> Gateway::CreateClientProcessor(tcp::socket socket) {
    ClientProcess procesor{
        .socket = std::move(socket)
    };

    co_await procesor();

    co_return;
}


