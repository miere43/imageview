#include "direct2d.hpp"

bool Direct2D::initialize(HWND hwnd)
{
    HRESULT hr;

    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(options));

    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&device);
    if (FAILED(hr))
        return false;

    device->GetDesktopDpi(&dpi_x, &dpi_y);

    RECT client_area = { 0 };
    GetClientRect(hwnd, &client_area);

    D2D1_SIZE_U render_target_size = D2D1::SizeU(
        client_area.right - client_area.left, 
        client_area.bottom - client_area.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props = D2D1::HwndRenderTargetProperties(hwnd, render_target_size);

    hr = device->CreateHwndRenderTarget(props, hwnd_props, &render_target);
    if (FAILED(hr))
        goto releaseD2D1FactoryAndFail;

    return true;

releaseD2D1FactoryAndFail:
    safe_release(device);
    return false;
}

bool Direct2D::discard()
{
    safe_release(device);
    safe_release(render_target);

    return true;
}

