#pragma once
#include <d2d1_1.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

template<typename T>
inline void safe_release(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

struct Direct2D
{
    float dpi_x = 72.0f;
    float dpi_y = 72.0f;

    ID2D1Factory1* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    ID2D1HwndRenderTarget* render_target = nullptr;

    bool initialize(HWND hwnd);
    bool discard();
};