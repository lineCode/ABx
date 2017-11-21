#pragma once

#include <mutex>

class Maintenance
{
private:
    std::mutex lock_;
    enum MaintenanceStatus
    {
        StatusRunnig,
        StatusTerminated
    };
    MaintenanceStatus status_;
    void CleanCacheTask();
public:
    Maintenance() :
        status_(StatusTerminated)
    {}
    ~Maintenance() = default;

    void Run();
    void Stop();

    static Maintenance Instance;
};

