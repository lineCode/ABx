#pragma once

namespace Utils {

template <typename T>
class IdGenerator
{
public:
    IdGenerator() :
        id_(std::numeric_limits<T>::min())
    { }
    ~IdGenerator() = default;
    T Next()
    {
        // When used with unit32_t shouldn't rotate for 136 years if every second is a new ID generated.
        if (id_ >= std::numeric_limits<T>::max())
            id_ = std::numeric_limits<T>::min();
        return ++id_;
    }
private:
    T id_;
};

}