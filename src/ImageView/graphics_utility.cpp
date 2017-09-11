#include "graphics_utility.hpp"


ID2D1Factory1* Graphics_Utility::d2d1 = nullptr;
IDWriteFactory* Graphics_Utility::dwrite = nullptr;
IWICImagingFactory* Graphics_Utility::wic = nullptr;


bool Graphics_Utility::initialize()
{
    HRESULT hr;

    // Initialize Direct2D
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(options));

#if _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
    options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&d2d1);
    if (FAILED(hr))
        return false;

    // Initialize DirectWrite
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite);
    if (FAILED(hr))
        goto fail;

    // Initialize WIC
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (FAILED(hr))
        return false;

    return true;

fail:
    safe_release(dwrite);
    safe_release(d2d1);
    return false;
}

bool Graphics_Utility::shutdown()
{
    safe_release(wic);
    safe_release(d2d1);
    safe_release(dwrite);

    return true;
}

ID2D1HwndRenderTarget* Graphics_Utility::create_hwnd_render_target(HWND hwnd)
{
    RECT client_area;
    GetClientRect(hwnd, &client_area);

    D2D1_SIZE_U render_target_size = D2D1::SizeU(
        client_area.right - client_area.left,
        client_area.bottom - client_area.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props = D2D1::HwndRenderTargetProperties(hwnd, render_target_size);

    ID2D1HwndRenderTarget* render_target;
    HRESULT hr = d2d1->CreateHwndRenderTarget(props, hwnd_props, &render_target);
    if (FAILED(hr))
        return nullptr;

    return render_target;
}
