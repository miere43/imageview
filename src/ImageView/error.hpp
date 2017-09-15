#pragma once
#include <Windows.h>

#define E_VERIFY(cond) \
    do { \
        if (!(cond)) { \
            __debugbreak(); \
            return; \
        } \
    } while (0)

#define E_VERIFY_R(cond, ret_val) \
    do { \
        if (!(cond)) { \
            __debugbreak(); \
            return (ret_val); \
        } \
    } while (0)

#define E_VERIFY_NULL(cond) \
    do { \
        if ((cond) == nullptr) { \
            __debugbreak(); \
            return; \
        } \
    } while (0)

#define E_VERIFY_NULL_R(cond, ret_val) \
    do { \
        if ((cond) == nullptr) { \
            __debugbreak(); \
            return (ret_val); \
        } \
    } while (0)

void report_error(const wchar_t* format, ...);
void error_box(HWND hwnd, HRESULT hr);
