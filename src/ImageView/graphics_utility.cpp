#include "graphics_utility.hpp"
#include "error.hpp"

ID2D1Factory1* Graphics_Utility::d2d1 = nullptr;
IDWriteFactory* Graphics_Utility::dwrite = nullptr;
IWICImagingFactory* Graphics_Utility::wic = nullptr;

HRESULT Graphics_Utility::initialize(const wchar_t** out_error_message)
{
    E_VERIFY_NULL_R(out_error_message, E_INVALIDARG);

    HRESULT hr;

    // Initialize Direct2D
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(options));

#if _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_ERROR;
#else
    options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&d2d1);
    if (FAILED(hr)) {
        *out_error_message = L"Unable to create Direct2D factory";
        return hr;
    }

    // Initialize DirectWrite
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite);
    if (FAILED(hr)) {
        *out_error_message = L"Unable to create DirectWrite factory";
        goto fail;
    }

    // Initialize WIC
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
    if (FAILED(hr)) {
        *out_error_message = L"Unable to create WIC instance";
        goto fail;
    }

    return S_OK;

fail:
    safe_release(wic);
    safe_release(d2d1);
    safe_release(dwrite);

    return hr;
}

bool Graphics_Utility::shutdown()
{
    safe_release(wic);
    safe_release(d2d1);
    safe_release(dwrite);

    return true;
}

HRESULT Graphics_Utility::create_hwnd_render_target(HWND hwnd, ID2D1HwndRenderTarget** render_target)
{
    E_VERIFY_R(hwnd != 0, E_INVALIDARG);
    E_VERIFY_NULL_R(render_target, E_INVALIDARG);

    RECT client_area;
    GetClientRect(hwnd, &client_area);

    D2D1_SIZE_U render_target_size = D2D1::SizeU(
        client_area.right - client_area.left,
        client_area.bottom - client_area.top);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props = D2D1::HwndRenderTargetProperties(hwnd, render_target_size);

    return d2d1->CreateHwndRenderTarget(props, hwnd_props, render_target);
}