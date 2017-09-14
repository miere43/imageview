#pragma once
#include "allocator.hpp"

// Represents zero-terminated wide string.
struct String
{
    wchar_t* data = nullptr;
    int count = 0;

    String();
    String(wchar_t* data, int count);

    int index_of(wchar_t c) const;
    int last_index_of(wchar_t c) const;
    bool ends_with(wchar_t c) const;
    bool ends_with(const wchar_t* cstring) const;

    size_t calc_size() const ;
    size_t calc_size_no_zero_terminator() const;
    //bool copy_data_to_buffer(wchar_t* buffer, size_t buffer_size, )
    String substring(int start, int length = -1, IAllocator* allocator = g_standard_allocator) const;

    static bool is_null_or_empty(const String& string);
    static bool is_null(const String& string);
    static bool equals(const String& a, const String& b);

    static String allocate_string_of_length(int length, IAllocator* allocator = g_standard_allocator);
    static String duplicate(const wchar_t* string, int string_length, IAllocator* allocator = g_standard_allocator);

    static String join(wchar_t seperator, const String string_array[], size_t string_array_length, IAllocator* allocator = g_standard_allocator);

    // Just reference, not duplicate of string or something.
    static String reference_to_const_wchar_t(const wchar_t* string);

    static const String null;
};