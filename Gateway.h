//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
// 

#ifndef TCP_TUNNEL_GATEWAY_H
#define TCP_TUNNEL_GATEWAY_H
#include "asio_pch.h"
#include "TunnelListener.h"
#include <unordered_map>
#include <memory>

class Gateway
{
public:
    tcp::acceptor acceptor;
    std::unordered_map<tcp::socket, std::unique_ptr<TunnelListener>> listeners;



};


#endif //TCP_TUNNEL_GATEWAY_H
