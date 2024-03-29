#include "stdafx.h"
#include "DataProvider.h"
#include "ConfigManager.h"
#include "NavigationMesh.h"
#include "IONavMesh.h"
#include "Logger.h"
#include "IOTerrain.h"
#include "Terrain.h"
#include "Model.h"
#include "IOModel.h"
#include "IOScript.h"
#include "Subsystems.h"

namespace IO {

DataProvider::DataProvider()
{
    // Add Importer
    AddImporter<Navigation::NavigationMesh, IO::IONavMesh>();
    AddImporter<Game::Terrain, IO::IOTerrain>();
    AddImporter<Game::Model, IO::IOModel>();
    AddImporter<Game::Script, IO::IOScript>();
}

std::string DataProvider::GetDataFile(const std::string& name) const
{
    static std::string dataDir = (*GetSubsystem<ConfigManager>())[ConfigManager::DataDir].GetString();
    return dataDir + name;
}

const std::string& DataProvider::GetDataDir() const
{
    return (*GetSubsystem<ConfigManager>())[ConfigManager::DataDir].GetString();
}

bool DataProvider::FileExists(const std::string& name) const
{
    return Utils::FileExists(name);
}

std::string DataProvider::GetFile(const std::string& name) const
{
    if (Utils::FileExists(name))
        return name;
    std::string n = GetDataFile(name);
    if (Utils::FileExists(n))
        return n;
    return name;
}

void DataProvider::CleanCache()
{
    if (cache_.size() == 0)
        return;

#ifdef _DEBUG
    LOG_DEBUG << "Cleaning cache" << std::endl;
#endif
    // Delete all assets that are only owned by the cache
    auto i = cache_.begin();
    while ((i = std::find_if(i, cache_.end(), [](const auto& current) -> bool
    {
        return current.second.use_count() == 1;
    })) != cache_.end())
        cache_.erase(i++);
}

}
