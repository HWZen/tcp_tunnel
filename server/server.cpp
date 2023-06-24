//
// Created by HWZen on 2023/6/22.
// Copyright (c) 2023 HWZen All rights reserved.
// MIT License
//
#include "Gateway.h"
int main(){
    Gateway gateway{ 8080 };
    gateway.spin();
}