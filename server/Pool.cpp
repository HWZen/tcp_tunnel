//
// Created by HWZ on 2023/6/23.
//

#include "Pool.h"

template<class Ty>
Pool<Ty> &Pool<Ty>::GetInstance() {
    static Pool instance;
    return instance;
}

template<class Ty>
bool Pool<Ty>::HasAcceptor(uint16_t port) {
    std::lock_guard lg{acceptorMutex};
    return acceptors.contains(port);
}

template<class Ty>
bool Pool<Ty>::AddAcceptor(uint16_t port, std::shared_ptr<Ty> acceptor) {
    std::lock_guard lg{acceptorMutex};
    if (acceptors.contains(port)){
        return false;
    }
    acceptors.emplace(port, std::move(acceptor));
    return true;
}

template<class Ty>
std::shared_ptr<Ty> Pool<Ty>::GetAcceptor(uint16_t port) {
    if (!HasAcceptor(port))
        return {};
    return acceptors.at(port);
}

template<class Ty>
bool Pool<Ty>::ReleaseAcceptor(uint16_t port) {
    std::lock_guard lg{acceptorMutex};
    if (!acceptors.contains(port)){
        return false;
    }
    acceptors.erase(port);
    return true;
}

#include "asio/ip/tcp.hpp"
template class Pool<asio::ip::tcp::acceptor>;

