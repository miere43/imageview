#include "direct2d.hpp"

bool Direct2D::initialize(HWND hwnd)
{
    HRESULT hr;

    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(options));

    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&d2d1);
    if (FAILED(hr))
        return false;

    d2d1->GetDesktopDpi(&dpi_x, &dpi_y);

    RECT client_area = { 0 };
    GetClientRect(hwnd, &client_area);

    D2D1_SIZE_U render_target_size = D2D1::SizeU(
        client_area.right - client_area.left, 
        client_area.bottom - client_area.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props = D2D1::HwndRenderTargetProperties(hwnd, render_target_size);

    hr = d2d1->CreateHwndRenderTarget(props, hwnd_props, &render_target);
    if (FAILED(hr))
        goto fail;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite);
    if (FAILED(hr))
        goto fail;

    return true;

fail:
    safe_release(dwrite);
    safe_release(d2d1);
    return false;
}

bool Direct2D::discard()
{
    safe_release(d2d1);
    safe_release(render_target);

    return true;
}

