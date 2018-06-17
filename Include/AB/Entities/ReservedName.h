#pragma once

#include <AB/Entities/Entity.h>
#include <AB/Entities/Limits.h>

namespace AB {
namespace Entities {

constexpr auto KEY_RESERVED_NAMES = "reserved_names";

struct ReservedName : Entity
{
    static constexpr const char* KEY()
    {
        return KEY_RESERVED_NAMES;
    }
    template<typename S>
    void serialize(S& s)
    {
        s.ext(*this, BaseClass<Entity>{});
        s.text1b(name, Limits::MAX_RESERVED_NAME);
        s.value1b(isReserved);
    }

    std::string name;
    bool isReserved = false;
};

}
}