#include "stdafx.h"
#include "Application.h"
#include "Scheduler.h"
#include "Dispatcher.h"
#include "ProtocolGame.h"
#include "ProtocolLogin.h"
#include "ProtocolAdmin.h"
#include "ProtocolStatus.h"
#include "ConfigManager.h"
#include "Task.h"
#include <thread>
#include <chrono>
#include "Logger.h"
#include "Utils.h"
#include "GameManager.h"
#include "Rsa.h"
#include <functional>
#include "Random.h"
#include <ostream>
#include <iostream>
#include "Connection.h"
#include "Database.h"

Application* gApplication = nullptr;

#ifdef  _WIN32
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    assert(gApplication);

#ifdef _DEBUG
    LOG_DEBUG << "Got signal " << dwCtrlType << std::endl;
#endif

    switch (dwCtrlType)
    {
    case CTRL_CLOSE_EVENT:                  // Close button or End Task
        gApplication->Stop();
        return TRUE;
    case CTRL_C_EVENT:                      // Ctrl+C
    {
        // Either it stops or it crashes...
        std::cout << "Stopping server? [y/n]: ";
        std::string answer;
        std::cin >> answer;
        if (answer.compare("y") == 0)
        {
            Asynch::Dispatcher::Instance.Add(
                Asynch::CreateTask(std::bind(&Application::Stop, gApplication))
            );
        }
        return TRUE;
    }
    default:
        return FALSE;
    }
}
#endif

Application::Application() :
    loaderUniqueLock_(loaderLock_)
{
    assert(gApplication == nullptr);
    gApplication = this;
}

Application::~Application()
{
    Asynch::Scheduler::Instance.Stop();
    Asynch::Dispatcher::Instance.Stop();
    DB::Database* db = DB::Database::Instance();
}

bool Application::Initialize(int argc, char** argv)
{
    using namespace std::chrono_literals;
#ifdef _WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandlerRoutine, TRUE);
#endif
#ifdef _WIN32
    char buff[MAX_PATH];
    GetModuleFileNameA(NULL, buff, MAX_PATH);
    std::string aux(buff);
#else
    std::string aux(argv[0]);
#endif
    size_t pos = aux.find_last_of("\\/");
    path_ = aux.substr(0, pos);
    for (int i = 0; i < argc; i++)
    {
        arguments_.push_back(std::string(argv[i]));
    }

    Asynch::Dispatcher::Instance.Start();
    Asynch::Scheduler::Instance.Start();

    Asynch::Dispatcher::Instance.Add(Asynch::CreateTask(std::bind(&Application::MainLoader, this)));
    loaderSignal_.wait(loaderUniqueLock_);
    std::this_thread::sleep_for(100ms);

    if (!serviceManager_.IsRunning())
        LOG_ERROR << "No services running" << std::endl;

    return serviceManager_.IsRunning();
}

void Application::MainLoader()
{
    int64_t startLoading = Utils::AbTick();

    LOG_INFO << "Loading..." << std::endl;

    LOG_INFO << "Loading configuration...";
    ConfigManager::Instance.Load(path_ + "/" + CONFIG_FILE);
    LOG_INFO << "[done]" << std::endl;

    LOG_INFO << "Initializing RNG...";
    Utils::Random::Instance.Initialize();
    LOG_INFO << "[done]" << std::endl;

    // DB ----------------
    LOG_INFO << "Creating DB connection...";
    DB::Database* db = DB::Database::Instance();
    if (db == nullptr || !db->IsConnected())
    {
        LOG_ERROR << "Database connection failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    // TODO:
    LOG_INFO << "[done]" << std::endl;

//    LOG_INFO << "Checking DB schema version...";
    // TODO:
//    LOG_INFO << "[done]" << std::endl;

    serviceManager_.Add<Net::ProtocolLogin>(ConfigManager::Instance[ConfigManager::Key::LoginPort].GetInt());
    serviceManager_.Add<Net::ProtocolAdmin>(ConfigManager::Instance[ConfigManager::Key::AdminPort].GetInt());
    serviceManager_.Add<Net::ProtocolStatus>(ConfigManager::Instance[ConfigManager::Key::StatusPort].GetInt());
    serviceManager_.Add<Net::ProtocolGame>(ConfigManager::Instance[ConfigManager::Key::GamePort].GetInt());

    PrintServerInfo();

    LOG_INFO << "Loading done in " << (Utils::AbTick() - startLoading) / (1000) << " sec." << std::endl;

    Game::GameManager::Instance.Start(&serviceManager_);

    // Notify we are ready
    loaderSignal_.notify_all();
}

void Application::PrintServerInfo()
{
    std::list<uint16_t> ports = serviceManager_.GetPorts();
    LOG_INFO << "Local ports: ";
    while (ports.size())
    {
        LOG_INFO << ports.front() << "\t";
        ports.pop_front();
    }
    LOG_INFO << std::endl;
}

void Application::Run()
{
    LOG_INFO << "Server is running" << std::endl;
    serviceManager_.Run();
}

void Application::Stop()
{
    LOG_INFO << "Server is shutting down" << std::endl;
    // Before serviceManager_.Stop()
    Net::ConnectionManager::GetInstance()->CloseAll();

    serviceManager_.Stop();
}