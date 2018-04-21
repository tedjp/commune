#include <iostream>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include "commune.h"
#include "safefd.h"

using commune::Commune;

using ::std::runtime_error;
using ::std::strerror;
using ::std::string;

#define MULTICAST_ADDR "ff02::262d"

Commune::Commune() {
    sock_ = commune::SafeFD(::socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));

    if (sock_ == -1)
        throw runtime_error(string("socket() failed: ") + strerror(errno));

    const int yes = 1;

    // Neither RECVERR nor REUSEADDR is strictly necessary, so ignore
    // the return value.
    setsockopt(sock_, IPPROTO_IPV6, IPV6_RECVERR, &yes, sizeof(yes));
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(DEFAULT_GROUP);
    sin6.sin6_addr = in6addr_any;
    sin6.sin6_scope_id = 0; // any

    if (bind(sock_, (const struct sockaddr*)&sin6, sizeof(sin6)) != 0)
        throw std::runtime_error(string("Failed to bind: ") + strerror(errno));

    struct if_nameindex *nameindex = if_nameindex();

    if (!nameindex)
        throw runtime_error("Failed to retrieve interfaces");

    struct ipv6_mreq mreq/* = {
        .ipv6mr_multiaddr = {{{0}}},
    }*/;
    memset(&mreq, 0, sizeof(mreq)); // maybe overkill

    if (1 != inet_pton(AF_INET6, MULTICAST_ADDR, &mreq.ipv6mr_multiaddr))
        throw std::runtime_error("Failed to parse multicast group address");

    unsigned joined = 0;
    for (unsigned i = 0; nameindex[i].if_index != 0; ++i) {
        // Not all interfaces are INET6-capable.
        // We could use getifaddrs and cross-reference interface names,
        // but just try to join on all.
        mreq.ipv6mr_interface = nameindex[i].if_index;

        if (setsockopt(sock_, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq,
                sizeof(mreq)) == 0) {
            ++joined;
        } else {
            // This is normal on some interfaces
            std::cerr << "Failed to join on interface " <<
                nameindex[i].if_index << ": " << strerror(errno) << std::endl;
        }
    }

    if_freenameindex(nameindex);

    if (joined == 0)
        throw std::runtime_error("Unable to join on any interfaces");
}

int Commune::event_fd() const {
    return sock_;
}

void Commune::onEvent() {
    // 1452 is max payload of UDP-on-IPv6-on-Ethernet
    // Jumboframe L2 transports allow higher values, eg.
    // UDP max is 2^16 octets.
    char buf[1452];
    struct sockaddr_storage ss;
    socklen_t socklen = sizeof(ss);
    ssize_t len = recvfrom(sock_, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&ss), &socklen);
    if (len < 0) {
        // FIXME: Could be an error, call again with MSG_ERRQUEUE
        throw std::runtime_error(string("error reading from socket: ") + strerror(errno));
    }

    // Quick hack; assume MESG
    ssize_t txtlen = len - 5;
    if (txtlen > 0)
        receiveMessage("(placeholder)", string(buf+5, txtlen));
}
