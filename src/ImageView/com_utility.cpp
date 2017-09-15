#include "com_utility.hpp"

const wchar_t * hresult_to_string(HRESULT hr)
{
#define VAL(ec, msg) case ec: return L##msg
    switch (hr)
    {
        VAL(S_OK, "Operation successful");
        VAL(E_NOTIMPL, "Not implemented");
        VAL(E_NOINTERFACE, "No such interface supported");
        VAL(E_POINTER, "Pointer that is not valid");
        VAL(E_ABORT, "Operation aborted");
        VAL(E_FAIL, "Unspecified failure");
        VAL(E_UNEXPECTED, "Unexpected failure");
        VAL(E_ACCESSDENIED, "General access denied error");
        VAL(E_HANDLE, "Handle that is not valid");
        VAL(E_OUTOFMEMORY, "Failed to allocate necessary memory");
        VAL(E_INVALIDARG, "One or more arguments are not valid");

        VAL(__HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), "Invalid window handle");

        // Wincodec
        VAL(WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT, "The pixel format is not supported"); // same as D2DERR_UNSUPPORTED_PIXEL_FORMAT

        // Direct2D
        VAL(D2DERR_EXCEEDS_MAX_BITMAP_SIZE, "The requested size is larger than the guaranteed supported texture size");
        VAL(D2DERR_WRONG_STATE, "The object was not in the correct state to process the method");
        VAL(D2DERR_NOT_INITIALIZED, "The object has not yet been initialized");
        VAL(D2DERR_UNSUPPORTED_OPERATION, "The requested opertion is not supported");

        default: return L"Unknown error";
    }
#undef VAL
}
