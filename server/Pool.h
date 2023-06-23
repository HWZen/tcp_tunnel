//
// Created by HWZ on 2023/6/23.
//

#ifndef TCP_TUNNEL_POOL_H
#define TCP_TUNNEL_POOL_H
#include <unordered_map>
#include <mutex>

template<class Ty>
class Pool  {
    Pool() = default;
public:
    static Pool& GetInstance();

    bool HasAcceptor(uint16_t port);

    bool AddAcceptor(uint16_t port, std::shared_ptr<Ty> acceptor);

    std::shared_ptr<Ty> GetAcceptor(uint16_t port);

    bool ReleaseAcceptor(uint16_t port);

private:
    std::unordered_map<uint16_t, std::shared_ptr<Ty>> acceptors;
    std::mutex acceptorMutex;
};




#endif //TCP_TUNNEL_POOL_H
