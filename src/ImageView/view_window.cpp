#include <Windows.h>
#include <windowsx.h>
#include <wchar.h>

#include "string_builder.hpp"
#include "view_window.hpp"
#include "defer.hpp"


LRESULT __stdcall wndproc_proxy(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

enum class View_Window_Message : UINT
{
    Show_After_Entering_Event_Loop = WM_USER + 1,
};

enum class View_Menu_Item : int
{
    None = 0,
    Open_File = 1,
    Quit_App = 2,
};

enum class View_Hotkey : int
{
    None = 0,
    View_Prev = 1,
    View_Next = 2,
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
        if (atom == 0) {
            report_error(L"Unable to register window class.\n");
            return false;
        }
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

    // Get desktop size
    int window_x = params.window_x;
    int window_y = params.window_y;

    {
        HMONITOR primary_monitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitor_info;
        monitor_info.cbSize = sizeof(monitor_info);

        if (GetMonitorInfoW(primary_monitor, &monitor_info) && monitor_info.dwFlags == MONITORINFOF_PRIMARY)
        {
            int desktop_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
            int desktop_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

            if (desktop_width > 0 && desktop_height > 0)
            {
                this->desktop_width = desktop_width;
                this->desktop_height = desktop_height;
            }
            else
            {
                report_error(L"Unable to get full desktop size.\n");
            }

            int desktop_work_width = monitor_info.rcWork.right - monitor_info.rcMonitor.left;
            int desktop_work_height = monitor_info.rcWork.bottom - monitor_info.rcWork.top;

            if (desktop_work_width > 0 && desktop_work_height > 0)
            {
                this->desktop_work_width = desktop_work_width;
                this->desktop_work_height = desktop_work_height;
            }
            else
            {
                report_error(L"Unable to get desktop work size.\n");
            }
        }
        else
        {
            report_error(L"Unable to get primary monitor info.\n");
        }
    }

    // Center window if necessary
    if (window_x == -1 && window_y == -1)
    {
        window_x = (desktop_width / 2) - (window_width / 2);
        window_y = (desktop_height / 2) - (window_height / 2);
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

    if (hwnd == 0) {
        report_error(L"Unable to create window.");
        return false;
    }

    SetLastError(0);
    SetWindowLongPtrW(hwnd, 0, (LONG_PTR)this);
    if (GetLastError() != 0) {
        report_error(L"Unable to set window data.\n");
        return false;
    }

    // Initialize graphics resources
    wic = params.wic;
    d2d1 = params.d2d1;
    dwrite = params.dwrite;

    if (wic == nullptr || d2d1 == nullptr || dwrite == nullptr) {
        report_error(L"One or more graphics objects are not assigned.\n");
        return false;
    }

    wic->AddRef();
    d2d1->AddRef();
    dwrite->AddRef();

    // Create render target
    hr = Graphics_Utility::create_hwnd_render_target(hwnd, &g);
    if (FAILED(hr)) {
        report_error(L"Unable to create window render target.\n");
        return false;
    }

    // Create hotkeys
    RegisterHotKey(hwnd, (int)View_Hotkey::View_Prev, 0, VK_LEFT);
    RegisterHotKey(hwnd, (int)View_Hotkey::View_Next, 0, VK_RIGHT);

    // Create view menu
    view_menu = CreatePopupMenu();
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Open_File, L"Open...");
    AppendMenuW(view_menu, MF_SEPARATOR, (UINT_PTR)View_Menu_Item::None, nullptr);
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Quit_App, L"Quit");

    // Create default text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        32.0f,
        L"",
        &default_text_format);
    if (FAILED(hr)) {
        report_error(L"Unable to create text format.\n");
        return false;
    }

    hr = g->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &default_text_foreground_brush);
    if (FAILED(hr)) {
        report_error(L"Unable to create text foreground brush.\n");
        return false;
    }

    UpdateWindow(hwnd);

    if (params.show_after_entering_event_loop)
        PostMessageW(hwnd, (UINT)View_Window_Message::Show_After_Entering_Event_Loop, 0, 0);

    this->hwnd = hwnd;
    this->wic = params.wic;

    return (initialized = true);
}

bool View_Window::shutdown()
{
    safe_release(wic);
    safe_release(d2d1);
    safe_release(dwrite);
    safe_release(current_image_direct2d);
    safe_release(decoder);
    safe_release(g);
    safe_release(default_text_foreground_brush);
    safe_release(default_text_format);


    DestroyWindow(hwnd);
    hwnd = 0;

    return true;
}

static bool image_filter(const WIN32_FIND_DATA& data, void* userdata)
{
    static const wchar_t* image_extensions[] = { L".jpg", L".png", L".gif", L".bmp", nullptr };
    
    bool is_file = data.dwFileAttributes != INVALID_FILE_ATTRIBUTES && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    if (!is_file)
        return false;

    String file_name = String::reference_to_const_wchar_t(data.cFileName);

    int i = 0;
    const wchar_t* ext = nullptr;
    while (ext = image_extensions[i++])
    {
        if (file_name.ends_with(ext))
            return true;
    }

    return false;
}

String View_Window::get_file_info_absolute_path(const String& folder, File_Info* file_info, IAllocator* allocator)
{
    const String strings[] = { folder, file_info->path };
    const size_t num = ARRAYSIZE(strings);

    return String::join(L'/', strings, num, allocator);
}

void View_Window::load_path(const String& file_path)
{
    String folder_path;
    if (!File_System_Utility::extract_folder_path(file_path, &folder_path))
        __debugbreak();

    Folder_Files files;
    bool ok = File_System_Utility::get_folder_files(&files, folder_path.data, image_filter, nullptr);

    if (!ok)
        __debugbreak();

    current_folder = folder_path;
    current_files = files.files;
    current_file_index = 0;

    void* prev = g_temporary_allocator->current;
    String file_name;
    if (!File_System_Utility::extract_file_name_from_path(file_path, &file_name, g_temporary_allocator))
        __debugbreak();

    int index = find_file_info_by_path(file_name);
    if (index == -1)
        __debugbreak();

    g_temporary_allocator->current = prev;

    current_file_index = index;
    view_file_index(index);
}

void View_Window::view_prev()
{
    int index = current_file_index - 1;
    if (index >= 0)
        view_file_index(index);
    else
        view_file_index(current_files.count + index);
}

void View_Window::view_next()
{
    int index = current_file_index + 1;
    if (index < current_files.count)
        view_file_index(index);
}

void View_Window::view_file_index(int index)
{
    E_VERIFY(index >= 0);
    if (current_file_index < 0)
        return;
    if (index > current_files.count)
        return;

    release_current_image();
    current_file_index = index;

    void* current = g_temporary_allocator->current;
    String full_path = get_file_info_absolute_path(current_folder, &current_files.data[current_file_index], g_temporary_allocator);
    if (String::is_null(full_path))
        __debugbreak();

    IWICBitmapDecoder* decoder = create_decoder_from_file_path(full_path.data);
    g_temporary_allocator->current = current;
    
    if (decoder == nullptr)
        return;

    if (set_current_image(decoder))
    {
        update_view_title();
    }
}

void View_Window::update_view_title()
{
    String_Builder title;
    void* prev = g_temporary_allocator->current;
    title.allocator = g_temporary_allocator;

    title.append_format(L"(%i/%i) %s", current_file_index, current_files.count, current_files.data[current_file_index].path);
    title.append_char(L'\0');

    SetWindowTextW(hwnd, title.buffer);
    g_temporary_allocator->deallocate(title.buffer);
    g_temporary_allocator->current = prev;
}

bool View_Window::set_current_image(IWICBitmapDecoder* bitmap_decoder)
{
    if (bitmap_decoder == nullptr)
        return false;
    if (!release_current_image())
        return false;

    HRESULT hr;
    decoder = bitmap_decoder;

    IWICBitmapFrameDecode* bitmap_frame = nullptr;
    hr = bitmap_decoder->GetFrame(0, &bitmap_frame);

    WICPixelFormatGUID pixel_format;
    hr = bitmap_frame->GetPixelFormat(&pixel_format);
    if (FAILED(hr))
        return false;

    IWICFormatConverter* converter = nullptr;
    IWICBitmapSource* actual_source = bitmap_frame;
    if (pixel_format != GUID_WICPixelFormat32bppPBGRA)
    {
        hr = wic->CreateFormatConverter(&converter);
        if (FAILED(hr))
            return false;

        converter->Initialize(bitmap_frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);
        actual_source = converter;
    }

    hr = g->CreateBitmapFromWicBitmap(actual_source, &current_image_direct2d);
    if (FAILED(hr))
        __debugbreak();

    if (converter != nullptr)
        converter->Release();

    D2D1_SIZE_F image_size = current_image_direct2d->GetSize();
    resize_window((int)image_size.width, (int)image_size.height, (int)Center_Window | (int)Maximize_If_Too_Big);

    InvalidateRect(hwnd, nullptr, true);
    bitmap_frame->Release();

    return true;
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
//
//bool View_Window::close_current_image()
//{
//    safe_release(current_image_direct2d);
//    safe_release(current_image_wic);
//
//    InvalidateRect(hwnd, nullptr, false);
//
//    return true;
//}

bool View_Window::release_current_image()
{
    safe_release(current_image_direct2d);
    safe_release(decoder);

    return true;
}

bool View_Window::handle_open_file_action()
{
    HRESULT hr = 0;
    IFileOpenDialog* dialog = nullptr;
    IShellItemArray* items = nullptr;
    IShellItem* item = nullptr;
    defer (
        safe_release(item);
        safe_release(items);
        safe_release(dialog);
    );

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

    load_path(String::reference_to_const_wchar_t(path));
    CoTaskMemFree(path);

    return true;
}

int View_Window::find_file_info_by_path(const String& path)
{
    if (String::is_null_or_empty(path))
        return false;

    for (int i = 0; i < current_files.count; ++i)
    {
        const File_Info& file_info = current_files.data[i];

        if (String::equals(path, file_info.path))
            return i;
    }

    return -1;
}

IWICBitmapDecoder* View_Window::create_decoder_from_file_path(const wchar_t* path)
{
    if (path == nullptr)
        return nullptr;

    HRESULT hr;
    IWICBitmapDecoder* decoder = nullptr;

    hr = wic->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    return decoder;
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

            g->Resize(D2D1::SizeU(new_width, new_height));

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
            if (current_image_direct2d == nullptr)
            {
                g->BeginDraw();
                g->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

                default_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                default_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                
                g->DrawTextW(L"Please load image", wcslen(L"Please load image"), default_text_format,
                    client_area_as_rectf(), default_text_foreground_brush);

                g->EndDraw();
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

            g->BeginDraw();
            g->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f));

            g->DrawBitmap(current_image_direct2d, dest_rect);

            g->EndDraw();

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
                case View_Menu_Item::Quit_App:
                    this->shutdown();
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
        case WM_HOTKEY:
        {
            View_Hotkey hotkey = (View_Hotkey)wParam;
            switch (hotkey)
            {
                case View_Hotkey::View_Prev:
                    view_prev();
                    break;
                case View_Hotkey::View_Next:
                    view_next();
                    break;
            }

            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

D2D1_RECT_F View_Window::client_area_as_rectf()
{
    RECT client_area;
    GetClientRect(hwnd, &client_area);

    return D2D1::RectF(0.0f, 0.0f, (float)client_area.right, (float)client_area.bottom);
}

void View_Window::resize_window(int new_width, int new_height, int flags)
{
    DWORD swp_flags = SWP_NOREDRAW | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOCOPYBITS;
    int new_x = 0;
    int new_y = 0;

    WINDOWPLACEMENT window_info = { 0 };

    GetWindowPlacement(hwnd, &window_info);

    if (window_info.showCmd == SW_MAXIMIZE)
        swp_flags |= SWP_NOSIZE | SWP_NOMOVE;

    if (flags & Center_Window)
    {
        new_x = (desktop_width / 2) - (new_width / 2);
        new_y = (desktop_height / 2) - (new_height / 2);
    }
    else
    {
        swp_flags |= SWP_NOMOVE;
    }

    if (flags & Maximize_If_Too_Big)
    {
        if (new_width >= desktop_work_width || new_height >= desktop_work_height)
            ShowWindow(hwnd, SW_MAXIMIZE);
    }

    SetWindowPos(hwnd, 0, new_x, new_y, new_width, new_height, swp_flags);
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
