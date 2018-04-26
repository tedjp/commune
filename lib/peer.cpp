#include "peer.h"

using std::string;
using namespace commune;

NetID::NetID(const struct sockaddr_in6 *addr):
    addr_(*addr)
{
    char name[5];

    int len = snprintf(name, sizeof(name), "%02hhx%02hhx",
            addr->sin6_addr.s6_addr[14],
            addr->sin6_addr.s6_addr[15]);
    if (len != 4)
        throw std::runtime_error("Failed to format network address");

    str_ = name;

    if (addr->sin6_port == htons(1818))
        return;

    char port[5];
    int len = snprintf(port, sizeof(port), "%04hx", addr->sin6_port);
    if (len != 4) {
        memcpy(port, "????", 5);
    }

    str_ += ":";
    str_ += port;
}

Peer::Peer(const struct sockaddr_in6* addr):
    netid_(addr)
    nick_()
{}

void Peer::name(const string& new_name) {
    // XXX: Sanitize to remove control & newline (& whitespace?) characters?
    name_ = new_name;
}

string Peer::name() const {
    string namestr = string("[") + netid_.string() + "]";

    if (nick_.empty())
        return namestr;

    namestr += " " + nick_;
}
