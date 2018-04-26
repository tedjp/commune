#pragma once

namespace commune {
class Channel {
    // Local human-readable alias for this channel
    // Perhaps presented alongside the number, eg.
    // "Public (1818)"
    std::string alias_;

    // Nickname to use in this channel (maybe empty)
    std::string nick_;

    int socket_ = -1;
    const uint16_t port_ = 0;

public:
    Channel(
            uint18_t port,
            const std::string& nick = std::string(),
            const std::string& alias = std::string());

    int fd() const { return socket_; }
    //bool handleSocketEvent(Glib::IOCondition io_condition);
};
} // namespace commune
