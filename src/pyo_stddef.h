#pragma once


#include <stddef.h>

#ifndef unreachable
#ifdef __GNUC__ // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#else
#ifdef _MSC_VER // MSVC

#define unreachable() (__assume(false))

#else
#error "No deffinition for unreachable"
#endif
#endif
#endif