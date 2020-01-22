#pragma once

#ifdef _MSC_VER
#define WINDOWS
#include <Windows.h>
#undef DrawText
#else
#include <unistd.h>
#endif