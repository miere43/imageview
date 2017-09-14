#pragma once
#include "string.hpp"
#include <Windows.h>

//struct Error
//{
//    HRESULT error_code;
//    String  error_message;
//
//    static Error* fromHRESULT(HRESULT error_code);
//};

template<typename T>
struct Expected
{
    union
    {
        T value;
        HRESULT error_code;
    };

    bool has_value = false;

    static Expected<T> error(HRESULT error_code);
};

template<typename T>
inline Expected<T> Expected<T>::error(HRESULT error_code)
{
    Expected<T> result;
    result.has_value = false;
    result.error_code = error_code;

    return result;
}
