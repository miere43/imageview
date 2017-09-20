#include <stdarg.h>
#include <wchar.h>
#include <Windows.h>

#include "error.hpp"
#include "string_builder.hpp"
#include "com_utility.hpp"
#include "windows_utility.hpp"


void report_error(const wchar_t * format, ...)
{
    String_Builder sb{ g_temporary_allocator };
    Temporary_Allocator_Guard g;

    va_list args;
    va_start(args, format);
    sb.begin();
    sb.append_format(format, args);
    if (!sb.end())
        __debugbreak(); // Cannot format error message.
    va_end(args);

    OutputDebugStringW(sb.buffer);
    __debugbreak();
}

void report_win32_error(const wchar_t* format, ...)
{
    String_Builder sb{ g_temporary_allocator };
    Temporary_Allocator_Guard g;

    va_list args;
    va_start(args, format);
    sb.begin();
    sb.append_format(L"[win32 error: %d] ", GetLastError());
    sb.append_format(format, args);
    if (!sb.end())
        __debugbreak(); // Cannot format error message.
    va_end(args);

#if _DEBUG
    OutputDebugStringW(sb.buffer);
    __debugbreak();
#else
    MessageBoxW(0, sb.buffer, L"Unexpected error", MB_ICONERROR);
#endif
}

void debug(const wchar_t* format, ...)
{
    String_Builder sb{ g_temporary_allocator };
    Temporary_Allocator_Guard g;

    va_list args;
    va_start(args, format);

    sb.begin();
    sb.append_format(format, args);
    if (!sb.end())
        __debugbreak(); // Cannot format error message.
    va_end(args);

#if _DEBUG
    OutputDebugStringW(sb.buffer);
#else
    MessageBoxW(0, sb.buffer, L"Unexpected error", MB_ICONERROR);
#endif
}

void log_error(const wchar_t* file, int line, const wchar_t* format, ...)
{
    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    sb.begin();
    sb.append_format(L"[%s:%d] ** ERROR **: \"", file, line);
    
    va_list args;
    va_start(args, format);
    sb.append_format(format, args);
    va_end(args);

    sb.append_string(L"\".\n");
    if (!sb.end())
    {
        E_DEBUGBREAK();
        return;
    }

    OutputDebugStringW(sb.buffer);
}

void log_hresult_error(const wchar_t * file, int line, HRESULT hr, const wchar_t * format, ...)
{
    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    sb.begin();
    sb.append_format(L"[%s:%d] ** ERROR **: \"", file, line);

    va_list args;
    va_start(args, format);
    sb.append_format(format, args);
    va_end(args);

    sb.append_format(L"\" (HRESULT is %#010x: ", hr);
    sb.append_string(hresult_to_string(hr));
    sb.append_string(L")\n");

    if (!sb.end())
    {
        E_DEBUGBREAK();
        return;
    }

    OutputDebugStringW(sb.buffer);
}

void log_win32_error(const wchar_t* file, int line, DWORD error_code, const wchar_t* format, ...)
{
    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    sb.begin();
    sb.append_format(L"[%s:%d] ** ERROR **: \"", file, line);

    va_list args;
    va_start(args, format);
    sb.append_format(format, args);
    va_end(args);

    sb.append_format(L"\" (Error is %#010x: ", error_code);
    Windows_Utility::error_to_string(error_code, sb);
    sb.append_string(L")\n");

    if (!sb.end())
    {
        E_DEBUGBREAK();
        return;
    }

    OutputDebugStringW(sb.buffer);
}

void log_last_win32_error(const wchar_t * file, int line, const wchar_t * format, ...)
{
    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    sb.begin();
    sb.append_format(L"[%s:%d] ** ERROR **: \"", file, line);

    va_list args;
    va_start(args, format);
    sb.append_format(format, args);
    va_end(args);

    DWORD last_error = GetLastError();

    sb.append_format(L"\" (Last error is %#010x: ", last_error);
    Windows_Utility::error_to_string(last_error, sb);
    sb.append_string(L")\n");

    if (!sb.end())
    {
        E_DEBUGBREAK();
        return;
    }

    OutputDebugStringW(sb.buffer);
}

void error_box(HWND hwnd, HRESULT hr)
{
    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    sb.begin();
    sb.append_string(L"Error: ");
    sb.append_string(hresult_to_string(hr));
    sb.append_string(L"\n\n");
    sb.append_format(L"HRESULT: %#010x", hr);
    if (sb.end())
        MessageBoxW(hwnd, sb.buffer, L"Error", MB_OK | MB_ICONERROR);
    else
        MessageBoxW(hwnd, L"Got an error, but cannot format it.", L"Error", MB_OK | MB_ICONERROR);
}
