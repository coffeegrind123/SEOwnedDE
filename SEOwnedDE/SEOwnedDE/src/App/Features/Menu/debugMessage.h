#pragma once

#include <iostream>
#include <windows.h>
#include <cstdio>

// Disable debug output for SEOwnedDE integration (no separate console)
// #define DEBUG

#ifdef DEBUG
#define LOGHEX(name, val) std::cout << name << ": " << std::hex << val << std::endl;
#define ALLOCCONSOLE()\
{\
    AllocConsole();\
    FILE* pCout;\
    freopen_s(&pCout, "CONOUT$", "w", stdout);\
}
#define FREECONSOLE()\
{\
    fclose(stdout);\
    FreeConsole();\
}
#else
#define LOGHEX(name, val)
#define ALLOCCONSOLE()
#define FREECONSOLE()
#endif