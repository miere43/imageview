#include "string.hpp"
#include "error.hpp"
#include <wchar.h>

const String String::null;


String::String()
{}

String::String(wchar_t * data, int count)
    : data(data),
      count(count)
{}

int String::index_of(wchar_t c) const
{
    if (is_null_or_empty(*this))
        return -1;

    for (int i = 0; i < count; ++i)
        if (data[i] == c)
            return i;

    return -1;
}

int String::last_index_of(wchar_t c) const
{
    if (is_null_or_empty(*this))
        return -1;

    for (int i = count - 1; i >= 0; --i)
        if (data[i] == c)
            return i;

    return -1;
}

bool String::ends_with(wchar_t c) const
{
    if (is_null_or_empty(*this))
        return false;

    return data[count - 1] == c;
}

bool String::ends_with(const wchar_t* cstring) const
{
    if (is_null_or_empty(*this) || cstring == nullptr)
        return false;

    int cstring_length = (int)wcslen(cstring);
    if (count < cstring_length)
        return false;

    for (int i = count - cstring_length; i < count; ++i)
        if (data[i] != *cstring++)
            return false;

    return true;
}

size_t String::calc_size() const
{
    if (is_null_or_empty(*this))
        return 0;

    return sizeof(wchar_t) * (count + 1);
}

size_t String::calc_size_no_zero_terminator() const
{
    if (is_null_or_empty(*this))
        return 0;

    return sizeof(wchar_t) * count;
}

bool String::is_null_or_empty(const String& string)
{
    return (&string == nullptr || string.data == nullptr || string.count <= 0);
}

bool String::is_null(const String & string)
{
    return &string == nullptr || string.data == nullptr;
}

bool String::equals(const String& a, const String& b)
{
    if (a.data == nullptr && b.data == nullptr)
        return true;
    if (a.count != b.count)
        return false;

    return 0 == wcsncmp(a.data, b.data, a.count);
}

String String::substring(int start, int length, IAllocator* allocator) const
{
    E_VERIFY_R(start >= 0, String::null);
    E_VERIFY_R(length >= -1, String::null);
    E_VERIFY_NULL_R(allocator, String::null);

    if (start >= count)
        return String::null;
    if (length < 0)
        length = this->count;
    if (start + length >= count)
        length = length - start;

    String result = String::allocate_string_of_length(length, allocator);
    if (String::is_null(result))
        return String::null;

    memcpy(result.data, &data[start], length * sizeof(wchar_t));
    result.data[length] = L'\0';

    return result;
}

String String::allocate_string_of_length(int length, IAllocator* allocator)
{
    E_VERIFY_R(length >= 0, String::null);
    E_VERIFY_NULL_R(allocator, String::null);

    size_t string_size = sizeof(wchar_t) * (length + 1);
    wchar_t* data = (wchar_t*)allocator->allocate(string_size);
    if (data == nullptr)
        return String::null;

    return String(data, length);
}

String String::duplicate(const wchar_t* string, int string_length, IAllocator* allocator)
{
    E_VERIFY_NULL_R(string, String::null);
    E_VERIFY_R(string_length >= 0, String::null);
    E_VERIFY_NULL_R(allocator, String::null);

    String new_string = allocate_string_of_length(string_length, allocator);
    if (is_null(new_string))
        return String::null;

    for (int i = 0; i < string_length; ++i)
        new_string.data[i] = string[i];
    
    new_string.data[string_length] = L'\0';
    return new_string;
}

String String::join(wchar_t seperator, const String string_array[], size_t string_array_length, IAllocator* allocator)
{
    E_VERIFY_NULL_R(string_array, String::null);
    E_VERIFY_NULL_R(allocator, String::null);

    int count = 0;
    for (size_t i = 0; i < string_array_length; ++i)
    {
        count += string_array[i].count;
        if (i != string_array_length - 1)
            count += 1; // Count seperator
    }

    if (count == 0)
        return String::null;

    String result = String::allocate_string_of_length(count, allocator);
    if (is_null(result))
        return String::null;

    wchar_t* data = result.data;
    for (size_t i = 0; i < string_array_length; ++i)
    {
        const String* s = &string_array[i];

        memcpy(data, s->data, sizeof(wchar_t) * s->count);
        data += s->count;

        if (i != string_array_length - 1)
            *data++ = seperator;
    }
    *data = L'\0';
    
    return result;
}

String String::reference_to_const_wchar_t(const wchar_t* string)
{
    return String((wchar_t*)string, string == nullptr ? 0 : (int)wcslen(string));
}
