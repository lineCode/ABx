#pragma once

#include "Variant.h"
#include "SimpleConfigManager.h"

class ConfigManager : public IO::SimpleConfigManager
{
public:
    enum Key : size_t
    {
        Machine,
        ServerName,
        ServerID,
        Location,
        GamePort,
        GameHost,
        GameIP,
        ServerKeys,

        LogDir,
        DataDir,
        RecordingsDir,
        RecordGames,

        DataServerHost,
        DataServerPort,
        MessageServerHost,
        MessageServerPort,

        MaxPacketsPerSecond,

        LoginTries,
        LoginTimeout,
        LoginRetryTimeout,

        Behaviours,
        AIServer,
        AIServerIp,
        AIServerPort,
    };
public:
    ConfigManager();
    ~ConfigManager() = default;

    Utils::Variant& operator[](Key key)
    {
        return config_[key];
    }

    bool Load(const std::string& file);

    Utils::VariantMap config_;
};

