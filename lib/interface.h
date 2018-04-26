#pragma once

#include <cstdint>
#include <string>
#include <map>

namespace commune {

class Interface {
    std::string name_;
    unsigned int index_;
    Scope scope_;
    std::string_ default_nick_;

    std::map<uint16_t, Channel> channels_;

public:
    // RFC 7346
    enum class Scope: uint_fast8_t {
        Reserved_0,
        Interface,
        Link,
        Realm,
        Admin,
        Site,
        Unassigned_6,
        Unassigned_7,
        Organization,
        Unassigned_9,
        Unassigned_A,
        Unassigned_B,
        Unassigned_C,
        Unassigned_D,
        Global,
        Reserved_F,
    };

    Interface(const std::string& name, Scope scope = Scope::Organization);

    const std::string& default_nick() const {
        return default_nick_;
    }

    void default_nick(const std::string& new_default_nick) {
        default_nick_ = new_default_nick;
    }
};

} // namespace commune
