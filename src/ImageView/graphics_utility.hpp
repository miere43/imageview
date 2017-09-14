#pragma once
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

#include "com_utility.hpp"
#include "expected.hpp"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

struct Graphics_Utility
{
    static ID2D1Factory1* d2d1;
    static IDWriteFactory* dwrite;
    static IWICImagingFactory* wic;
  
    static bool initialize();
    static bool shutdown();

    static HRESULT create_hwnd_render_target(HWND hwnd, ID2D1HwndRenderTarget** render_target);
};
