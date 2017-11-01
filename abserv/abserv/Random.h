#pragma once

#include <rng.h>
#include <mutex>
#include <limits>

namespace Utils {

class Random
{
private:
    rng::rng64 gen_;
    std::mutex lock_;
public:
    Random() = default;
    ~Random() {}

    void Initialize();
    bool GetBool();
    /// 0..1
    float GetFloat();

    template <typename T>
    T Get()
    {
        std::lock_guard<std::mutex> lock(lock_);
        return static_cast<T>(gen_());
    }
    /// Get value from 0..max
    template <typename T>
    T Get(T max)
    {
        return static_cast<T>((float)Get<T>() / (float)std::numeric_limits<T>::max() * (float)(max + 1));
    }
    /// Get value from min..max
    template <typename T>
    T Get(T min, T max)
    {
        assert(max > min);
        return Get<T>(max - min) + min;
    }

    static Random Instance;
};

}