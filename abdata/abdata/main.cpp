// abdata.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include "Application.h"
#include "Version.h"
#include "Logo.h"
#include "ServiceConfig.h"
#if !defined(WIN_SERVICE)

#include <iostream>
#include <csignal>     /* signal, raise, sig_atomic_t */
#include <functional>
#include "MiniDump.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#   define CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

namespace {
std::function<void(int)> shutdown_handler;
void signal_handler(int signal)
{
    shutdown_handler(signal);
}
} // namespace

static void ShowLogo()
{
    std::cout << "This is " << SERVER_PRODUCT_NAME << std::endl;
    std::cout << "Version " << SERVER_VERSION_MAJOR << "." << SERVER_VERSION_MINOR <<
        " (" << __DATE__ << " " << __TIME__ << ")";
#ifdef _DEBUG
    std::cout << " DEBUG";
#endif
    std::cout << std::endl;
    std::cout << "(C) 2017-" << SERVER_YEAR << std::endl;
    std::cout << std::endl;

    std::cout << AB_CONSOLE_LOGO << std::endl;

    std::cout << std::endl;
}

#ifdef AB_WINDOWS
static std::mutex gTermLock;
static std::condition_variable termSignal;
#endif

int main(int argc, char* argv[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
#if defined(_MSC_VER) && defined(WRITE_MINIBUMP)
    SetUnhandledExceptionFilter(System::UnhandledHandler);
#endif

    ShowLogo();

    std::signal(SIGINT, signal_handler);              // Ctrl+C
    std::signal(SIGTERM, signal_handler);
#ifdef AB_WINDOWS
    std::signal(SIGBREAK, signal_handler);            // X clicked
#endif

    {
        Application app;
        if (!app.InitializeA(argc, argv))
            return EXIT_FAILURE;
        shutdown_handler = [&app](int /*signal*/)
        {
#ifdef AB_WINDOWS
            std::unique_lock<std::mutex> lockUnique(gTermLock);
#endif
            app.Stop();
#ifdef AB_WINDOWS
            termSignal.wait(lockUnique);
#endif
        };
        app.Run();
    }
#ifdef AB_WINDOWS
    termSignal.notify_all();
#endif

    return EXIT_SUCCESS;
}

#else   // !defined(WIN_SERVICE)
// Internal name of the service
#define SERVICE_NAME             L"ABDataServer"
// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"AB Data Server"
#define SERVICE_DESCRIPTION      L"Forgotten Wars Data Server"
// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START
// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""
// The name of the account under which the service should run
// LocalService may not start because it does not have access to the directory
#define SERVICE_ACCOUNT          L"NT AUTHORITY\\LocalService"
// The password to the service account name
#define SERVICE_PASSWORD         NULL

#include "WinService.h"
AB_SERVICE_MAIN(System::WinService<Application>)
#endif // !defined(WIN_SERVICE)
