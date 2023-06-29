//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
//
#include "Agent.h"
#include <iostream>
int main() try{
    Agent agent("127.0.0.1", 5678, "10.238.57.130", 8080);
    agent.Connect();
    agent.Run();

}
catch (std::exception& e){
    std::cout << "catch exception: " << e.what();
    return 1;
}