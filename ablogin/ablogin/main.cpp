// ablogin.cpp : Definiert den Einstiegspunkt f�r die Konsolenanwendung.
//

#include "stdafx.h"
#include "Application.h"
#include "Version.h"

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

    std::cout << "##########  ######  ######" << std::endl;
    std::cout << "    ##          ##  ##" << std::endl;
    std::cout << "    ##  ######  ##  ##" << std::endl;
    std::cout << "    ##  ##      ##  ##" << std::endl;
    std::cout << "    ##  ##  ##  ##  ##" << std::endl;
    std::cout << "    ##  ##  ##  ##  ##" << std::endl;
    std::cout << "    ##  ##  ##  ##  ##" << std::endl;

    std::cout << std::endl;
}

int main(int argc, char** argv)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    ShowLogo();

    Application app;
    if (!app.Initialize(argc, argv))
        return EXIT_FAILURE;

    signal(SIGINT, signal_handler);              // Ctrl+C
    signal(SIGBREAK, signal_handler);            // X clicked
    shutdown_handler = [&app](int /*signal*/)
    {
        app.Stop();
    };

    app.Run();

    return EXIT_SUCCESS;
}
