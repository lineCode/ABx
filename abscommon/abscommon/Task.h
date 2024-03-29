#pragma once

#include <chrono>

namespace Asynch {

using the_clock = std::chrono::system_clock;

class Task
{
public:
    explicit Task(unsigned ms, const std::function<void(void)>& f) :
        expires_(true),
        function_(f)
    {
        expiration_ = the_clock::now() + std::chrono::milliseconds(ms);
    }
    explicit Task(const std::function<void(void)>& f) :
        expiration_(the_clock::time_point::max()),
        expires_(false),
        function_(f)
    {}
    ~Task() = default;

    /// Execute function
    void operator()()
    {
        function_();
    }

    void SetDontExpires()
    {
        expires_ = false;
    }
    bool IsExpired() const
    {
        if (!expires_)
            // Does not expire
            return false;
        return expiration_ < the_clock::now();
    }
protected:
    the_clock::time_point expiration_;
    bool expires_;
private:
    std::function<void(void)> function_;
};

/// Creates a task that does not expire.
inline Task* CreateTask(std::function<void(void)> f)
{
    return new Task(f);
}
/// Creates a new task that may expire.
/// \param ms Expiration
/// \param f Function
inline Task* CreateTask(unsigned ms, std::function<void(void)> f)
{
    return new Task(ms, f);
}

}
