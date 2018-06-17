//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "stdafx.h"

#if defined(_MSC_VER)

#include "StringUtils.h"
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <windows.h>
#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)
#include "Logger.h"
#include "MiniDump.h"

namespace System {

LONG UnhandledHandler(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    char fn[MAX_PATH];
    GetModuleFileNameA(nullptr, fn, MAX_PATH);
    std::string filename(fn);
    size_t pos = filename.find_last_of("\\/");
    filename = filename.substr(pos, std::string::npos);
    return WriteMiniDump(filename.c_str(), apExceptionInfo);
}

int WriteMiniDump(const char* applicationName, EXCEPTION_POINTERS* exceptionPointers)
{
    static bool miniDumpWritten = false;

    // In case of recursive or repeating exceptions, only write the dump once
    /// \todo This function should not allocate any dynamic memory
    if (miniDumpWritten)
        return EXCEPTION_EXECUTE_HANDLER;

    miniDumpWritten = true;

    MINIDUMP_EXCEPTION_INFORMATION info;
    info.ThreadId = GetCurrentThreadId();
    info.ExceptionPointers = (EXCEPTION_POINTERS*)exceptionPointers;
    info.ClientPointers = TRUE;

    std::chrono::time_point<std::chrono::system_clock> time_point;
    time_point = std::chrono::system_clock::now();
    std::time_t ttp = std::chrono::system_clock::to_time_t(time_point);
    tm p;
    localtime_s(&p, &ttp);
    char dateTime[50];
    strftime(dateTime, 50, "%Y-%m-%d-%H-%M-%S", (const tm*)&p);
    std::string dateTimeStr = std::string(dateTime);

    char curDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, curDir);
    std::string miniDumpDir = std::string(curDir) + std::string("\\dumps");

    std::string miniDumpName = miniDumpDir + std::string(applicationName) + "_" + dateTimeStr + ".dmp";

    CreateDirectoryA(miniDumpDir.c_str(), nullptr);
    HANDLE file = CreateFileA(miniDumpName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, 0, nullptr);

    BOOL success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
        file, MiniDumpWithDataSegs, &info, nullptr, nullptr);
    CloseHandle(file);

    if (success)
        LOG_ERROR << "An unexpected error occurred. A minidump was generated to " + miniDumpName;
    else
        LOG_ERROR << "An unexpected error occurred. Could not write minidump.";

    return EXCEPTION_EXECUTE_HANDLER;
}

#pragma comment(lib, "dbghelp.lib")

}

#endif