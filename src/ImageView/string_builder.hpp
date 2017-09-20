#pragma once
#include <stdarg.h>

#include "allocator.hpp"
#include "string.hpp"


struct String_Builder
{
    wchar_t* buffer = nullptr;
    int count = 0;
    int capacity = 0;
    IAllocator* allocator = nullptr;
    bool is_valid = false;
    bool is_begin_called = false;

    String_Builder(IAllocator* allocator = g_standard_allocator);

    void clear();

    void begin();
    bool append_char(wchar_t c);
    bool append_string(const String& string);
    bool append_string(const wchar_t* string);
    bool append_format(const wchar_t* format, ...);
    bool append_format(const wchar_t* format, va_list args);
    bool end(bool append_terminating_null = true);

    bool reserve(int required_capacity);
private:
    bool ensure_capacity(int required_capacity);
    inline int find_capacity(int cap) { return cap <= 5 ? 10 : cap * 2; }
};
