//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
//
#include "Gateway.h"
#include <iostream>
int main(int argc, char **argv) try{
    uint16_t port{8080};
    if (argc > 1)
        port = std::stoi(argv[1]);
    Gateway gateway{ port };
    gateway.spin();
}
catch (std::exception &e){
    std::cout << "Catch exception: " << e.what() << std::endl;

}