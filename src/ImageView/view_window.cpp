#include "view_window.hpp"
#include <Windows.h>
#include <windowsx.h>
#include "defer.hpp"

LRESULT __stdcall wndproc_proxy(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

enum class View_Window_Message : UINT
{
    Show_After_Entering_Event_Loop = WM_USER + 1,
};

enum class View_Menu_Item : int
{
    None = 0,
    Open_File = 1
};

bool View_Window::initialize(const View_Window_Init_Params& params)
{
    if (initialized)
        return true;

    HINSTANCE hInstance = GetModuleHandleW(0);
    HRESULT hr = 0;

    ATOM atom = 0;
    {
        // Register window class
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.hCursor = LoadCursorW(0, IDC_ARROW);
        wc.hIcon = LoadIconW(0, IDI_APPLICATION);
        wc.hInstance = hInstance;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = wndproc_proxy;
        wc.cbWndExtra = sizeof(this);
        wc.lpszClassName = L"View Image Window"; 

        atom = RegisterClassExW(&wc);
        if (atom == 0)
            return false;
    }

    // Adjust window size
    const DWORD style_flags_ex = 0;
    const DWORD style_flags    = WS_BORDER | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;

    int window_width;
    int window_height;
    {
        RECT window_rect = { 0 };

        window_rect.right  = params.window_client_area_width;
        window_rect.bottom = params.window_client_area_height;

        AdjustWindowRectEx(&window_rect, style_flags, false, style_flags_ex);
    
        window_width  = window_rect.right  - window_rect.left;
        window_height = window_rect.bottom - window_rect.top;
    }

    // Center window if necessary
    int window_x;
    int window_y;

    if (params.window_x == -1 && params.window_y == -1)
    {
        bool cannot_center_window = true;

        HMONITOR primary_monitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitor_info;
        monitor_info.cbSize = sizeof(monitor_info);

        if (GetMonitorInfoW(primary_monitor, &monitor_info) && monitor_info.dwFlags == MONITORINFOF_PRIMARY)
        {
            int desktop_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
            int desktop_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

            if (desktop_width > 0 && desktop_height > 0)
            {
                window_x = (desktop_width / 2) - (window_width / 2);
                window_y = (desktop_height / 2) - (window_height / 2);

                cannot_center_window = false;
            }
        }

        if (cannot_center_window)
        {
            window_x = 0;
            window_y = 0;
        }
    }
    else
    {
        window_x = params.window_x;
        window_y = params.window_y;
    }

    // Create window
    CREATESTRUCT create_struct = { 0 };
    create_struct.lpCreateParams = this;

    HWND hwnd = CreateWindowExW(
        style_flags_ex,
        (LPCWSTR)atom,
        L"Image View",
        style_flags,
        window_x,
        window_y,
        window_width,
        window_height,
        0,
        0,
        hInstance,
        &create_struct);

    if (hwnd == 0)
        return false;

    if (!direct2d.initialize(hwnd))
        return false;

    SetLastError(0);
    SetWindowLongPtrW(hwnd, 0, (LONG_PTR)this);
    if (GetLastError() != 0)
        return false;

    // Create view menu
    view_menu = CreatePopupMenu();
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Open_File, L"Open...");

    UpdateWindow(hwnd);

    if (params.show_after_entering_event_loop)
    {
        PostMessageW(hwnd, (UINT)View_Window_Message::Show_After_Entering_Event_Loop, 0, 0);
    }

    this->hwnd = hwnd;
    this->wic_factory = params.wic_factory;

    return (initialized = true);
}

bool View_Window::discard()
{
    direct2d.discard();
    safe_release(current_image_direct2d);

    return false;
}

bool View_Window::set_current_image(IWICBitmapSource* bitmap_source)
{
    if (bitmap_source == nullptr)
        return false;
    if (!release_current_image())
        return false;

    HRESULT hr;
    current_image_wic = bitmap_source;

    WICPixelFormatGUID pixel_format;
    hr = bitmap_source->GetPixelFormat(&pixel_format);
    if (FAILED(hr))
        return false;

    IWICFormatConverter* converter = nullptr;
    IWICBitmapSource* actual_source = bitmap_source;
    if (pixel_format != GUID_WICPixelFormat32bppPBGRA)
    {
        wic_factory->CreateFormatConverter(&converter);
        
        converter->Initialize(bitmap_source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);
        actual_source = converter;
    }

    hr = direct2d.render_target->CreateBitmapFromWicBitmap(actual_source, &current_image_direct2d);
    if (FAILED(hr))
        __debugbreak();

    if (converter != nullptr)
        converter->Release();

    InvalidateRect(hwnd, nullptr, true);

    return true;
}

bool View_Window::set_file_name(const wchar_t* file_name, size_t length)
{
    wchar_t temp[260];
    const wchar_t* title = L"Image View - ";
    const size_t title_length = wcslen(title);

    wcsncpy(temp, title, title_length);
    wcsncpy(&temp[title_length], file_name, length);
    temp[title_length + length] = L'\0';

    return !!SetWindowTextW(hwnd, temp);
}

bool View_Window::get_client_area(int* width, int* height)
{
    if (width == nullptr || height == nullptr)
        return false;

    RECT rect;
    if (!GetClientRect(hwnd, &rect))
        return false;

    *width  = rect.right - rect.left;
    *height = rect.bottom - rect.top;

    return true;
}

bool View_Window::release_current_image()
{
    safe_release(current_image_direct2d);
    safe_release(current_image_wic);

    return true;
}

bool View_Window::handle_open_file_action()
{
    HRESULT hr = 0;
    IFileOpenDialog* dialog = nullptr;
    IShellItemArray* items = nullptr;
    IShellItem* item = nullptr;
    defer ({
        safe_release(item);
        safe_release(items);
        safe_release(dialog);
    });

    // Create open file dialog
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (dialog == nullptr)
        return false;

    hr = dialog->SetOptions(FOS_ALLOWMULTISELECT | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
    hr = dialog->Show(hwnd);
    hr = dialog->GetResults(&items);

    if (items == nullptr)
        return false;

    hr = items->GetItemAt(0, &item);
    if (item == nullptr)
        return false;

    wchar_t* path = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    IWICBitmapSource* source = load_image_from_path(path);
    CoTaskMemFree(path);

    if (!set_current_image(source))
        return false;

    return true;
}

IWICBitmapSource* View_Window::load_image_from_path(const wchar_t* path)
{
    if (path == nullptr)
        return nullptr;

    HRESULT hr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* result = nullptr;

    hr = wic_factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (decoder == nullptr)
        goto release;

    hr = decoder->GetFrame(0, &result);
    if (result == nullptr)
        goto release;

    safe_release(decoder);
    return result;

release:
    safe_release(decoder);
    safe_release(result);

    return result;
}

int View_Window::enter_message_loop()
{
    MSG msg;
    int result;

    run_message_loop = true;

    while (run_message_loop && (result = GetMessageW(&msg, hwnd, 0, 0) != 0))
    {
        if (result == -1)
        {
            __debugbreak();
            continue;
        }

        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return msg.message == WM_QUIT ? msg.wParam : 0;
}

LRESULT View_Window::wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            return 0;
        }
        case WM_SIZE:
        {
            int new_width = LOWORD(lParam);
            int new_height = HIWORD(lParam);

            direct2d.render_target->Resize(D2D1::SizeU(new_width, new_height));

            return 0;
        }
        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            run_message_loop = false;

            return 0;
        }
        case WM_PAINT:
        {
            ID2D1RenderTarget* rt = direct2d.render_target;

            if (current_image_direct2d == nullptr)
            {
                rt->BeginDraw();
                rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
                rt->EndDraw();
                break;
            }

            int client_width, client_height;
            get_client_area(&client_width, &client_height);

            D2D1_SIZE_F image_size = current_image_direct2d->GetSize();
            D2D1_RECT_F dest_rect = D2D1::RectF(
                client_width / 2.0f - image_size.width / 2.0f,
                client_height / 2.0f - image_size.height / 2.0f,
                client_width / 2.0f + image_size.width / 2.0f,
                client_height / 2.0f + image_size.height / 2.0f
            );

            rt->BeginDraw();
            rt->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f));

            rt->DrawBitmap(current_image_direct2d, dest_rect);

            rt->EndDraw();

            ValidateRect(hwnd, nullptr);

            return 0;
        }
        case WM_CONTEXTMENU:
        {
            HWND target_window = (HWND)wParam;
            if (target_window != hwnd)
                break; // Pass it to default window proc.

            int menu_x = GET_X_LPARAM(lParam);
            int menu_y = GET_Y_LPARAM(lParam);

            SetForegroundWindow(hwnd);
            View_Menu_Item result = (View_Menu_Item)TrackPopupMenuEx(
                view_menu, 
                TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, 
                menu_x,
                menu_y,
                hwnd, 
                nullptr);

            switch (result)
            {
                case View_Menu_Item::None:
                    break;
                case View_Menu_Item::Open_File:
                    handle_open_file_action();
                    break;
                default:
                    __debugbreak(); // Unknown item
            }

            return 0;
        }
        case WM_ERASEBKGND:
        {
            return 0;
        }
        case (UINT)View_Window_Message::Show_After_Entering_Event_Loop:
        {
            ShowWindow(hwnd, SW_SHOWNORMAL);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT __stdcall wndproc_proxy(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            CREATESTRUCTW* create_struct = (CREATESTRUCTW*)lParam;
            if (create_struct == nullptr)
                return 1;

            View_Window* self = (View_Window*)create_struct->lpCreateParams;
            if (self == nullptr)
                return 1;

            return self->wndproc(hwnd, msg, wParam, lParam);
        }
    }

    View_Window* self = (View_Window*)GetWindowLongPtr(hwnd, 0);
    if (self == nullptr)
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    return self->wndproc(hwnd, msg, wParam, lParam);
}
