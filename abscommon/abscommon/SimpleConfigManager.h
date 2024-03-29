#pragma once

#include <lua.hpp>

namespace IO {

class SimpleConfigManager
{
private:
    lua_State* L;
    bool isLoaded;
public:
    SimpleConfigManager() :
        L(nullptr),
        isLoaded(false)
    { }
    ~SimpleConfigManager()
    {
        Close();
    }
    std::string GetGlobalString(const std::string& ident, const std::string& def);
    int64_t GetGlobalInt(const std::string& ident, int64_t def);
    float GetGlobalFloat(const std::string& ident, float def);
    bool GetGlobalBool(const std::string& ident, bool def);

    bool Load(const std::string& file);
    void Close()
    {
        if (L)
        {
            lua_close(L);
            L = nullptr;
        }
    }
};

}
