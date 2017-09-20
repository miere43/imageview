#pragma once
#include <Windows.h>

#include "string_builder.hpp"


struct Windows_Utility
{
    static HRESULT error_to_string(DWORD windows_error, String_Builder& builder);
};