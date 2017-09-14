#include <string.h>
#include <stdarg.h>
#include <wchar.h>

#include "string_builder.hpp"
#include "error.hpp"


bool String_Builder::append_char(wchar_t c)
{
    if (!ensure_capacity(count + 1)) {
        is_valid = true;
        return false;
    }

    buffer[count++] = c;
    return true;
}

bool String_Builder::append_string(const String& string)
{
    if (!ensure_capacity(string.count)) {
        is_valid = false;
        return false;
    }
    if (String::is_null_or_empty(string))
        return true;

    memcpy(&buffer[count], string.data, sizeof(wchar_t) * string.count);
    count += string.count;

    return true;
}

bool String_Builder::append_string(const wchar_t* string)
{
    return append_string(String::reference_to_const_wchar_t(string));
}

bool String_Builder::append_format(const wchar_t* format, ...)
{
    E_VERIFY_NULL_R(format, false);

    va_list ap;
    va_start(ap, format);

    int num_chars_required = _vscwprintf(format, ap);
    if (num_chars_required == -1) {
        is_valid = false;
        return false;
    }

    if (!ensure_capacity(count + num_chars_required)) {
        is_valid = false;
        return false;
    }

    wchar_t* data = &buffer[count];
    const size_t chars_left = (capacity - count);

    int num_written = vswprintf_s(data, chars_left, format, ap);
    if (num_written != num_chars_required) {
        is_valid = false;
        return false;
    }

    va_end(ap);

    count += num_chars_required;
    return true;
}

bool String_Builder::ensure_capacity(int required_capacity)
{
    E_VERIFY_R(required_capacity >= 0, false);
    if (required_capacity == 0)
        return true;
    if (capacity >= required_capacity)
        return true;

    int new_cap = find_capacity(required_capacity);
    if (buffer == nullptr) {
        buffer = (wchar_t*)allocator->allocate(new_cap);
        if (buffer == nullptr)
            return false;

        capacity = new_cap;
        count = 0;
        return true;
    }
    else
    {
        buffer = (wchar_t*)allocator->reallocate(buffer, new_cap);
        if (buffer == nullptr)
            return false;
    
        capacity = new_cap;
        return true;
    }
}
