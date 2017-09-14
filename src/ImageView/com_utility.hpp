#pragma once
#include <Windows.h>

template<typename T>
inline void safe_release(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

const wchar_t* hresult_to_string(HRESULT hr);
