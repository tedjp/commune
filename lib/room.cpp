#include "room.h"

using commune::Room;
using std::string;
using std::unordered_map;

static const string NONAME = string();

const std::string& Room::nick(const std::string& addr) const {
    auto nit = nicks_.find(addr);

    if (nit == nicks_.end())
        return NONAME;

    return nit->second;
}

void Room::setNick(std::string&& addr, std::string&& nick) {
#if __cplusplus >= 201703L
    nicks_.insert_or_assign(std::move(addr), std::move(nick));
#else
    auto ref = nicks_.find(addr);
    if (ref != nicks_.end())
        ref->second.assign(std::move(nick));
    else
        nicks_.emplace(std::move(addr), std::move(nick));
#endif
}

void Room::removeNickByAddress(const std::string& addrstr) {
    nicks_.erase(addrstr);
}
