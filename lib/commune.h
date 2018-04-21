#pragma once

#include <cstdint>
#include <string>

#include "safefd.h"

namespace commune {

static const uint16_t DEFAULT_GROUP = 1818;

class Commune {
public:
    Commune();

    Commune(const Commune&) = delete;

    Commune& operator=(const Commune&) = delete;

    void sendMessage(const std::string& msg, uint16_t group = DEFAULT_GROUP);

    // Event loop handling:
    // Have your event loop poll for input events on this fd.
    int event_fd() const;

    // Have your event loop call this (probably via std::mem_fun()
    // when there is activity on the event_fd().
    void onEvent();

protected:
    virtual void
    receiveMessage(__attribute__((unused)) const std::string& sender, __attribute__((unused)) const std::string& msg)
    {}

private:
    SafeFD sock_;
};

} // namespace commune
