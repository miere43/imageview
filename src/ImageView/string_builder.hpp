#pragma once
#include "allocator.hpp"
#include "string.hpp"

struct String_Builder
{
    wchar_t* buffer = nullptr;
    int count = 0;
    int capacity = 0;
    IAllocator* allocator = nullptr;

    bool append_char(wchar_t c);
    bool append_string(const String& string);
    bool append_string(const wchar_t* string);
    bool append_format(const wchar_t* format, ...);
private:
    bool ensure_capacity(int required_capacity);
    inline int find_capacity(int cap) { return cap <= 5 ? 10 : cap * 2; }
};