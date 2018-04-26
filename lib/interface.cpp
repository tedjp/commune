#include <net/if.h>
#include <memory>

#include "interface.h"

using std::string;
using std::unique_ptr;
using namespace commune;

Interface::Interface(const string& name, Scope scope):
    name_(name),
    index_(0),
    scope_(scope),
    default_nick_()
{
    unique_ptr<struct if_nameindex[], decltype(&if_freenameindex)>
        ifaces(if_nameindex(), &if_freenameindex);

    for (unsigned i = 0; ifaces[i].if_index != 0; ++i) {
        if (name == ifaces[i].if_name) {
            index_ = ifaces[i].if_index;
            return;
        }
    }

    throw std::runtime_error(string("Interface ") + name + " not found");
}
