#pragma once
#include <ostream>
#if defined(AB_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <Windows.h>
#endif

// https://misc.flogisoft.com/bash/tip_colors_and_formatting
namespace IO {
namespace Color {
    enum Code : short
    {
        // Foregrounds
#if !defined(AB_WINDOWS)
        FG_BLACK        = 30,
        FG_RED          = 31,
        FG_GREEN        = 32,
        FG_YELLOW       = 33,
        FG_BLUE         = 34,
        FG_MAGENTA      = 35,
        FG_CYAN         = 36,
        FG_LIGHTGREY    = 37,
        FG_DEFAULT      = 39,
        FG_DARKGREY     = 90,
        FG_LIGHTRED     = 91,
        FG_LIGHTGREEN   = 92,
        FG_LIGHTYELLOW  = 93,
        FG_LIGHTBLUE    = 94,
        FG_LIGHTMAGENTA = 95,
        FG_LIGHTCYAN    = 96,
        FG_WHITE        = 97,
#else
        FG_BLACK = 0,
        FG_WHITE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        FG_RED = FOREGROUND_RED,
        FG_GREEN = FOREGROUND_GREEN,
        FG_YELLOW = FOREGROUND_GREEN | FOREGROUND_RED,
        FG_BLUE = FOREGROUND_BLUE,
        FG_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
        FG_CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
        FG_LIGHTGREY = FOREGROUND_INTENSITY,
        FG_DEFAULT = FG_WHITE,
        FG_DARKGREY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        FG_LIGHTRED = FOREGROUND_INTENSITY | FOREGROUND_RED,
        FG_LIGHTGREEN = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
        FG_LIGHTYELLOW = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
        FG_LIGHTBLUE = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
        FG_LIGHTMAGENTA = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
        FG_LIGHTCYAN = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
#endif
        // Backgrounds
#if !defined(AB_WINDOWS)
        BG_BLACK        = 40,
        BG_RED          = 41,
        BG_GREEN        = 42,
        BG_YELLOW       = 43,
        BG_BLUE         = 44,
        BG_MAGENTA      = 45,
        BG_CYAN         = 46,
        BG_LIGHTGREY    = 47,
        BG_DEFAULT      = 49,
        BG_DARKGREY     = 100,
        BG_LIGHTRED     = 101,
        BG_LIGHTGREEN   = 102,
        BG_LIGHTYELLOW  = 103,
        BG_LIGHTBLUE    = 104,
        BG_LIGHTMAGENTA = 105,
        BG_LIGHTCYAN    = 106,
        BG_WHITE        = 107,
#else
        BG_BLACK = 0,
        BG_RED = BACKGROUND_RED,
        BG_GREEN = BACKGROUND_GREEN,
        BG_YELLOW = BACKGROUND_GREEN | BACKGROUND_RED,
        BG_BLUE = BACKGROUND_BLUE,
        BG_MAGENTA = BACKGROUND_RED | BACKGROUND_BLUE,
        BG_CYAN = BACKGROUND_GREEN | BACKGROUND_BLUE,
        BG_LIGHTGREY = BACKGROUND_INTENSITY,
        BG_DEFAULT = FG_BLACK,
        BG_DARKGREY = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
        BG_LIGHTRED = BACKGROUND_INTENSITY | BACKGROUND_RED,
        BG_LIGHTGREEN = BACKGROUND_INTENSITY | BACKGROUND_GREEN,
        BG_LIGHTYELLOW = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
        BG_LIGHTBLUE = BACKGROUND_INTENSITY | BACKGROUND_BLUE,
        BG_LIGHTMAGENTA = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
        BG_LIGHTCYAN = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
        BG_WHITE = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
#endif
    };
    class Modifier
    {
    public:
        explicit Modifier(Code code) : code_(code) {}
        friend std::ostream&
        operator << (std::ostream& os, const Modifier& mod)
        {
            return os << "\033[" << mod.code_ << "m";
        }
        Code code_;
    };
}
}
