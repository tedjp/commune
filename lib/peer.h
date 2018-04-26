#pragma once

#include <netinet/in.h>
#include <string>

namespace commune {

class NetID {
    const struct sockaddr_in6 addr_;
    std::string str_;

public:
    explicit NetID(const struct sockaddr_in6 *addr);

    // Intention: return the last 2 octets of the network
    // address in hex.
    // If the sender port is not 1818 it might be worth
    // appending the client's port number, eg.
    // 92d3:fbc3
    const std::string& string() const { return str_; }
};

class Peer {
    NetID netid_;
    std::string nick_;

public:
    explicit Peer(const struct sockaddr_in6*);

    std::string name() const;
    void name(const std::string& new_name);
};

} // namespace commune
