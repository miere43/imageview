#include <stdarg.h>
#include <wchar.h>
#include <Windows.h>

#include "error.hpp"
#include "string_builder.hpp"
#include "com_utility.hpp"


void report_error(const wchar_t * format, ...)
{
    static wchar_t buffer[512];

    va_list ap;
    va_start(ap, format);
    vswprintf_s(buffer, format, ap);
    va_end(ap);

    OutputDebugStringW(buffer);
    __debugbreak();
}

void error_box(HWND hwnd, HRESULT hr)
{
    String_Builder sb;
    sb.allocator = g_temporary_allocator;
    sb.is_valid = true;
    void* prev = g_temporary_allocator->current;

    sb.append_string(L"Error: ");
    sb.append_string(hresult_to_string(hr));
    sb.append_string(L"\n\n");
    sb.append_format(L"HRESULT: %#010x", hr);
    sb.append_char(L'\0');

    if (!sb.is_valid)
        MessageBoxW(hwnd, L"Got an error, but cannot format it.", L"Error", MB_OK | MB_ICONERROR);
    else
        MessageBoxW(hwnd, sb.buffer, L"Error", MB_OK | MB_ICONERROR);
    g_temporary_allocator->current = prev;
}
