#include <string.h>
#include <wchar.h>

#include "string_builder.hpp"
#include "error.hpp"


String_Builder::String_Builder(IAllocator * allocator)
{
    this->allocator = allocator;
}

void String_Builder::clear()
{
    E_VERIFY(is_begin_called == false); // Call 'end' first!

    count = 0;
    is_valid = true;
}

void String_Builder::begin()
{
    E_VERIFY(is_begin_called == false); // Call 'end' first!

    clear();
    is_begin_called = true;
}

bool String_Builder::append_char(wchar_t c)
{
    if (!ensure_capacity(count + 1)) {
        is_valid = false;
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

    wmemcpy(&buffer[count], string.data, string.count);
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

    va_list args;
    va_start(args, format);

    bool result = append_format(format, args);

    va_end(args);

    return result;
}

bool String_Builder::append_format(const wchar_t * format, va_list args)
{
    int num_chars_required = _vscwprintf(format, args);
    if (num_chars_required == -1) {
        is_valid = false;
        return false;
    }

    // _vscwprintf doesn't count terminating null, but vswprintf_s prints it.
    if (!ensure_capacity(count + num_chars_required + 1)) {
        is_valid = false;
        return false;
    }

    wchar_t* data = &buffer[count];
    const size_t chars_left = (capacity - count);

    int num_written = vswprintf_s(data, chars_left, format, args);
    if (num_written != num_chars_required) {
        is_valid = false;
        return false;
    }

    count += num_chars_required;
    return true;
}

bool String_Builder::end(bool append_terminating_null)
{
    E_VERIFY_R(is_begin_called == true, false); // Call 'begin' first!
    is_begin_called = false;

    if (is_valid && append_terminating_null)
        append_char(L'\0');

    return is_valid;
}

bool String_Builder::reserve(int required_capacity)
{
    E_VERIFY_R(required_capacity >= 0, false);
    if (capacity >= required_capacity)
        return true;

    wchar_t* new_buffer = (wchar_t*)allocator->reallocate(buffer, sizeof(wchar_t) * required_capacity);
    if (new_buffer == nullptr)
        return false;

    buffer = new_buffer;
    capacity = required_capacity;

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
