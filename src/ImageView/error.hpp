#pragma once
#include <Windows.h>

#ifdef _DEBUG
    #define E_DEBUGBREAK() __debugbreak()
#else
    #define E_DEBUGBREAK()
#endif

#define E_VERIFY(cond) \
    do { \
        if (!(cond)) { \
            E_DEBUGBREAK(); \
            return; \
        } \
    } while (0)

#define E_VERIFY_R(cond, ret_val) \
    do { \
        if (!(cond)) { \
            E_DEBUGBREAK(); \
            return (ret_val); \
        } \
    } while (0)

#define E_VERIFY_NULL(cond) \
    do { \
        if ((cond) == nullptr) { \
            E_DEBUGBREAK(); \
            return; \
        } \
    } while (0)

#define E_VERIFY_NULL_R(cond, ret_val) \
    do { \
        if ((cond) == nullptr) { \
            E_DEBUGBREAK(); \
            return (ret_val); \
        } \
    } while (0)

void error_box(HWND hwnd, HRESULT hr);
void debug(const wchar_t* format, ...);

void log_error(const wchar_t* file, int line, const wchar_t* format, ...);
void log_hresult_error(const wchar_t* file, int line, HRESULT hr, const wchar_t* format, ...);
void log_win32_error(const wchar_t* file, int line, DWORD error_code, const wchar_t* format, ...);
void log_last_win32_error(const wchar_t* file, int line, const wchar_t* format, ...);

#define LOG_ERROR(format, ...) \
    do { \
        log_error(__FILEW__, __LINE__, format, __VA_ARGS__); \
        E_DEBUGBREAK(); \
    } while (0)

#define LOG_HRESULT_ERROR(hr, format, ...) \
    do { \
        log_hresult_error(__FILEW__, __LINE__, hr, format, __VA_ARGS__); \
        E_DEBUGBREAK(); \
    } while (0)

#define LOG_WIN32_ERROR(code, format, ...) \
    do { \
        log_win32_error(__FILEW__, __LINE__, code, format, __VA_ARGS__); \
        E_DEBUGBREAK(); \
    } while (0)

#define LOG_LAST_WIN32_ERROR(format, ...) \
    do { \
        log_last_win32_error(__FILEW__, __LINE__, format, __VA_ARGS__); \
        E_DEBUGBREAK(); \
    } while (0)