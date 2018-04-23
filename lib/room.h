#include <cstdint>
#include <string>
#include <unordered_map>

namespace commune {

class Room {
private:
    const uint16_t port_;
    // Room name (local alias)
    std::string name_;
    std::string my_nick_;
    // Nicks are keyed by string so that they are keyed by both the address and
    // interface. It might be more efficient to key by struct in6_addr &
    // interface index.
    std::unordered_map<std::string, std::string> nicks_;

public:
    Room(uint16_t port):
        port_(port),
        name_(std::to_string(port))
    {}

    void myNick(std::string&& name) { my_nick_.assign(std::move(name)); }
    const std::string& myNick() const { return my_nick_; }

    const std::string& nick(const std::string& addr) const;
    void setNick(std::string&& addr, std::string&& nick);

    uint16_t port() const { return port_; }
    const std::string& name() const { return name_; }
    void name(std::string&& newName) { name_.assign(std::move(newName)); }
};

} // namespace commune
