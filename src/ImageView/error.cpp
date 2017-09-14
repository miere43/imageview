#include "error.hpp"
#include <wchar.h>
#include <stdarg.h>
#include <Windows.h>

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
