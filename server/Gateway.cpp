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
    TRACE_FUNC(logger);
    if (AcceptorPool::GetInstance().HasAcceptor(port)){
        LOG_ERROR(logger, "port already in use");
        throw std::runtime_error("port already in use");
    }
    listenAcceptor = std::make_shared<tcp::acceptor>(ioContext, tcp::endpoint(tcp::v4(), port));
    co_spawn(ioContext, Accept(), asio::detached);
}

awaitable<void> Gateway::Accept() {
    TRACE_FUNC(logger);
    for(;;){
        auto [ec, socket] = co_await listenAcceptor->async_accept(use_nothrow_awaitable);
        if (ec) {
            LOG_ERROR(logger, "accept error: {}", ec.message());
            break;
        }
        co_spawn(ioContext, CreateClientProcessor(std::move(socket)), asio::detached);
    }
    LOG_INFO(logger,"acceptor exit");
}

awaitable<void> Gateway::CreateClientProcessor(tcp::socket socket) {
    TRACE_FUNC(logger);
    ClientProcess procesor{std::move(socket)};

    co_await procesor();
    co_return;
}

size_t Gateway::spin() {
    TRACE_FUNC(logger);
    return ioContext.run();
}


