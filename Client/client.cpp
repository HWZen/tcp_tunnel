//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
//
#include "Agent.h"
#include <iostream>
int main(int argc, char **argv) try{
    std::string proxyAddress = "127.0.0.1";
    uint16_t proxyPort = 5678;
    std::string serverAddress = proxyAddress;
    uint16_t serverPort = 8080;
    uint16_t requestPort = proxyPort;
    if (argc > 1)
        proxyAddress = argv[1];
    if (argc > 2)
        proxyPort = std::stoi(argv[2]);
    if (argc > 3)
        serverAddress = argv[3];
    if (argc > 4)
        serverPort = std::stoi(argv[4]);
    if (argc > 5)
        requestPort = std::stoi(argv[5]);
    Agent agent(proxyAddress, proxyPort, serverAddress, serverPort, requestPort);
    agent.Connect();
    agent.Run();
}
catch (std::exception& e){
    std::cout << "catch exception: " << e.what();
    return 1;
}