#pragma once

#include <cstdint>
#include <netinet/in.h> // remove after pimpl refactor
#include <string>

#include "room.h"
#include "safefd.h"

namespace commune {

static const uint16_t DEFAULT_GROUP = 1818;

class Commune {
public:
    Commune();

    Commune(const Commune&) = delete;

    Commune& operator=(const Commune&) = delete;

    // Set nickname for all channels
    void setNick(const std::string& nick);

    void sendMessage(const std::string& msg, uint16_t group = DEFAULT_GROUP);

    // Event loop handling:
    // Have your event loop poll for input events on this fd.
    int event_fd() const;

    // Have your event loop call this (probably via std::mem_fn()
    // when there is activity on the event_fd().
    void onEvent();

    std::string sender(const struct sockaddr_in6*) const;

protected:
    virtual void
    receiveMessage(__attribute__((unused)) const std::string& sender, __attribute__((unused)) const std::string& msg)
    {}

    void send_net_msg(const std::string& type, const std::string& data = std::string());

private:
    SafeFD sock_;

    // TODO: Move to pImpl & remove associated includes
    struct sockaddr_in6 addr_;

    std::string nick_;
    // Future: multiple of these.
    Room room_;

    void dispatch(std::string&& sender, std::string&& msgtype, std::string&& msgbody);
};

} // namespace commune
