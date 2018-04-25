#include <iostream>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "commune.h"
#include "safefd.h"

namespace {

using std::string;

static string netID(const struct sockaddr_in6 *sin6) {
    char txt[sizeof("ffff")];

    if (4 != snprintf(
                txt,
                sizeof(txt),
                "%02x%02x",
                sin6->sin6_addr.s6_addr[14],
                sin6->sin6_addr.s6_addr[15]))
    {
        return string("????");
    }

    return string(txt, 4);
}

} // anonymous namespace

namespace commune {

using ::std::runtime_error;
using ::std::strerror;
using ::std::string;

#define MULTICAST_ADDR "ff02::262d"
#define VERSION 0

Commune::Commune():
    room_(DEFAULT_GROUP)
{
    sock_ = commune::SafeFD(::socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));

    if (sock_ == -1)
        throw runtime_error(string("socket() failed: ") + strerror(errno));

    const int yes = 1;

    // Neither RECVERR nor REUSEADDR is strictly necessary, so ignore
    // the return value.
    setsockopt(sock_, IPPROTO_IPV6, IPV6_RECVERR, &yes, sizeof(yes));
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    memset(&addr_, 0, sizeof(addr_));
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = htons(DEFAULT_GROUP);
    addr_.sin6_addr = in6addr_any;
    addr_.sin6_scope_id = 0; // any

    // TODO: If bind fails, bind a random port instead (for multi-user)
    if (bind(sock_, (const struct sockaddr*)&addr_, sizeof(addr_)) != 0)
        throw std::runtime_error(string("Failed to bind: ") + strerror(errno));

    std::unique_ptr<struct if_nameindex[], decltype(&if_freenameindex)>
        nameindex(if_nameindex(), &if_freenameindex);

    if (!nameindex)
        throw runtime_error("Failed to retrieve interfaces");

    struct ipv6_mreq mreq;
    memset(&mreq, 0, sizeof(mreq)); // maybe overkill

    if (1 != inet_pton(AF_INET6, MULTICAST_ADDR, &mreq.ipv6mr_multiaddr))
        throw std::runtime_error("Failed to parse multicast group address");

    addr_.sin6_addr = mreq.ipv6mr_multiaddr;

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

    nameindex.reset();

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
    struct sockaddr_in6 sin6;
    socklen_t socklen = sizeof(sin6);
    ssize_t len = recvfrom(sock_, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&sin6), &socklen);
    if (len < 0) {
        // FIXME: Could be an error, call again with MSG_ERRQUEUE
        throw std::runtime_error(string("error reading from socket: ") + strerror(errno));
    }

    if (socklen != sizeof(sin6))
        throw std::runtime_error("Unexpected address length");

    if (len < 5) {
        std::cerr << "Message too short, ignoring" << std::endl;
        return;
    }

    if (buf[0] != VERSION) {
        std::cerr << "Unrecognized version " << (unsigned) buf[0] << std::endl;
        return;
    }

    std::string msgtype(buf + 1, 4);
    std::string msgbody(buf + 5, len - 5);

    if (msgtype == "NICK" || msgtype == "GBYE") {
        // Tidy this up plz.
        char addrstr[INET6_ADDRSTRLEN];
        if (0 == getnameinfo(
                    reinterpret_cast<const struct sockaddr*>(&sin6),
                    sizeof(sin6),
                    addrstr, sizeof(addrstr),
                    nullptr, 0,
                    NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM))
        {
            if (msgtype == "NICK")
                onNickNotification(string(addrstr), std::move(msgbody));
            else if (msgtype == "GBYE")
                room_.removeNickByAddress(addrstr);

            return;
        } else {
            throw std::runtime_error("Can't figure out what sender's IP address is");
        }
    }

    dispatch(sender(&sin6), std::move(msgtype), std::move(msgbody));
}

void Commune::dispatch(std::string&& sender_name, std::string&& msgtype, std::string&& msgbody) {
    if (msgtype == "MESG")
        receiveMessage(sender_name, msgbody);
}

// Probably want to return a {netid,nick} pair.
std::string Commune::sender(const struct sockaddr_in6 *sin6) const {
    char addrstr[INET6_ADDRSTRLEN];

    if (0 != getnameinfo(
                reinterpret_cast<const struct sockaddr*>(sin6),
                sizeof(*sin6),
                addrstr, sizeof(addrstr),
                nullptr, 0,
                NI_DGRAM | NI_NUMERICHOST | NI_NUMERICSERV))
    {
        return "(unknown)";
    }

    string displayName = string("[" + netID(sin6) + "]");

    string name = room_.nick(string(addrstr));

    if (!name.empty())
        displayName += " " + name;

    return displayName;
}

void Commune::send_net_msg(const std::string& type, const std::string& body) {
    if (type.size() != 4)
        throw std::runtime_error(string(__func__) + ": message type " + type + " is not 4 chars");

    char netmsg[1452];

    netmsg[0] = '\0'; // version

    memcpy(netmsg + 1, type.data(), 4);

    size_t msglen = std::min(body.size(), sizeof(netmsg) - 5);
    memcpy(netmsg + 5, body.data(), msglen);
    msglen += 5;

    struct sockaddr_in6 dest = addr_;

    struct if_deleter {
        void operator()(struct if_nameindex *sifn) {
            if_freenameindex(sifn);
        }
    };

    auto ifaces = std::unique_ptr<struct if_nameindex[], decltype(&if_freenameindex)>(if_nameindex(), &if_freenameindex);

    bool ok = false;

    for (unsigned i = 0; ifaces[i].if_index != 0; ++i) {
        dest.sin6_scope_id = ifaces[i].if_index;
        if (sendto(sock_, netmsg, msglen, 0, reinterpret_cast<const struct sockaddr*>(&dest), sizeof(dest)) != -1)
            ok = true;
    }

    if (!ok)
        throw std::runtime_error(string("Failed to send ") + type + " message: " + strerror(errno));
}

void Commune::setNick(const string& nick) {
    send_net_msg("NICK", nick);
}

// XXX: API: remove/change group/room param
void Commune::sendMessage(const std::string& msg, uint16_t group) {
    send_net_msg("MESG", msg);
}

void
Commune::onNickNotification(std::string&& addrstr, std::string&& nick) {
    room_.setNick(std::move(addrstr), std::move(nick));
}

Commune::~Commune() {
    send_net_msg("GBYE");
    struct ipv6_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.ipv6mr_multiaddr = addr_.sin6_addr;
    setsockopt(sock_, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
}

} // namespace commune
