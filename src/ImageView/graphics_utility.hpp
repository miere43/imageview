#pragma once
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

template<typename T>
inline void safe_release(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

struct Graphics_Utility
{
    static ID2D1Factory1* d2d1;
    static IDWriteFactory* dwrite;
    static IWICImagingFactory* wic;
  
    static bool initialize();
    static bool shutdown();

    static ID2D1HwndRenderTarget* create_hwnd_render_target(HWND hwnd);
};
