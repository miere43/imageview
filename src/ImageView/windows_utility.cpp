#include <wchar.h>

#include "windows_utility.hpp"
#include "error.hpp"
#include "defer.hpp"

HRESULT Windows_Utility::error_to_string(DWORD windows_error, String_Builder& builder)
{
    E_VERIFY_R(builder.is_begin_called, E_INVALIDARG);
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER 
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_FROM_SYSTEM 
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    wchar_t* message = nullptr;
    DWORD chars = FormatMessageW(
        flags,
        nullptr,
        windows_error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (wchar_t*)&message,
        0,
        nullptr);

    if (chars == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    
    defer(LocalFree(message));
    
    // reserve +1 char for possible terminating null
    if (!builder.reserve(builder.count + (int)chars + 1))
        return E_OUTOFMEMORY;

    errno_t error = wmemcpy_s(
        builder.buffer + builder.count,
        builder.capacity - builder.count,
        message,
        chars);

    if (error == 0) {
        builder.count += (int)chars;
        return S_OK;
    }

    return E_FAIL;
}
