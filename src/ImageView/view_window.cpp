#include <Windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>

#include "string_builder.hpp"
#include "view_window.hpp"
#include "line_reader.hpp"
#include "defer.hpp"
#include "error.hpp"

#pragma comment(lib, "Shlwapi.lib")


LRESULT __stdcall wndproc_proxy(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT adjust_window_rect(int* width, int* height, DWORD window_flags, DWORD window_flags_ex);


static const DWORD windowed_display_mode_flags = WS_BORDER | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
static const DWORD fullscreen_display_mode_flags = WS_POPUP;


enum class View_Window_Message : UINT
{
    Show_After_Entering_Event_Loop = WM_USER + 1,
};

enum class View_Menu_Item : int
{
    None = 0,
    Open_File = 1,
    Quit_App = 2,
    Change_Display_Mode = 3,
    Copy_Filename_To_Clipboard = 4,
};
//
//enum class View_Hotkey : int
//{
//    None = 0,
//    View_Prev = VK_LEFT,
//    View_Next = VK_RIGHT,
//    View_Show_File_In_Explorer = VK_RETURN,
//    Fast_Quit = VK_ESCAPE,
//};

enum class View_Shortcut : WORD
{
    None = 0,
    View_Prev,
    View_First,
    View_Next,
    View_Last,
    View_Show_File_In_Explorer,
    Fast_Quit,
    Change_Display_Mode
};

bool View_Window::initialize(const View_Window_Init_Params& params, String command_line)
{
    if (initialized)
        return true;

    load_and_apply_settings();

    // Assign graphics resources
    wic = params.wic;
    d2d1 = params.d2d1;
    dwrite = params.dwrite;

    if (wic == nullptr || d2d1 == nullptr || dwrite == nullptr)
    {
        error_box(L"One or more graphics objects are not assigned.\n");
        return false;
    }

    wic->AddRef();
    d2d1->AddRef();
    dwrite->AddRef();

    HINSTANCE hInstance = GetModuleHandleW(0);
    HRESULT hr = 0;

    // Register window class
    ATOM atom = 0;
    {
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
            error_box(L"Unable to register window class.\n");
            return false;
        }
    }

    // Adjust window size
    const DWORD style_flags_ex = 0;

    int window_width = params.window_client_area_width;
    int window_height = params.window_client_area_height;

    hr = adjust_window_rect(&window_width, &window_height, windowed_display_mode_flags, style_flags_ex);
    if (FAILED(hr)) {
        error_box(hr);
        return false;
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
                LOG_ERROR(L"Unable to get full desktop size.\n");
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
                LOG_ERROR(L"Unable to get desktop work size.");
            }
        }
        else
        {
            LOG_LAST_WIN32_ERROR(L"Unable to get primary monitor info.");
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
        windowed_display_mode_flags, // @TODO
        window_x,
        window_y,
        window_width,
        window_height,
        0,
        0,
        hInstance,
        &create_struct);

    if (hwnd == 0) {
        error_box(L"Unable to create window.");
        return false;
    }

    SetLastError(0);
    SetWindowLongPtrW(hwnd, 0, (LONG_PTR)this);
    if (GetLastError() != 0) {
        error_box(L"Unable to set window data.\n");
        return false;
    }

    // Create view menu
    view_menu = CreatePopupMenu();
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Open_File, L"Open...");
    AppendMenuW(view_menu, MF_SEPARATOR, (UINT_PTR)View_Menu_Item::None, nullptr);
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Change_Display_Mode, L"Enter fullscreen");
    AppendMenuW(view_menu, MF_SEPARATOR, (UINT_PTR)View_Menu_Item::None, nullptr);
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Copy_Filename_To_Clipboard, L"Copy filename to clipboard");
    AppendMenuW(view_menu, MF_SEPARATOR, (UINT_PTR)View_Menu_Item::None, nullptr);
    AppendMenuW(view_menu, MF_STRING, (UINT_PTR)View_Menu_Item::Quit_App, L"Quit");

    UpdateWindow(hwnd);

    if (params.show_after_entering_event_loop)
        PostMessageW(hwnd, (UINT)View_Window_Message::Show_After_Entering_Event_Loop, 0, 0);
    
    this->hwnd = hwnd;
    this->wic = params.wic;

    hr = create_graphics_resources();
    if (FAILED(hr)) {
        error_box(hr);
        return false;
    }

    // Parse command line args
    int num_args;
    wchar_t** args;
    args = CommandLineToArgvW(command_line.data, &num_args);

    // Set default scaling
    // @TODO: read from settings.
    set_scaling_mode(Scaling_Mode::Fit_To_Window, 1.0f);

    // Initialize keyboard accelerator
    {
        static ACCEL accels[] = {
            { FVIRTKEY, VK_LEFT, (WORD)View_Shortcut::View_Prev },
            { FVIRTKEY, VK_RIGHT, (WORD)View_Shortcut::View_Next },
            { FVIRTKEY | FSHIFT, VK_LEFT, (WORD)View_Shortcut::View_First },
            { FVIRTKEY | FSHIFT, VK_RIGHT, (WORD)View_Shortcut::View_Last },
            { FVIRTKEY, VK_RETURN, (WORD)View_Shortcut::View_Show_File_In_Explorer },
            { FVIRTKEY, VK_ESCAPE, (WORD)View_Shortcut::Fast_Quit },
            { FVIRTKEY, VK_F11, (WORD)View_Shortcut::Change_Display_Mode },
        };

        kb_accel = CreateAcceleratorTableW(accels, ARRAYSIZE(accels));
        if (kb_accel == 0)
            LOG_LAST_WIN32_ERROR(L"Unable to create keyboard accelerator table");
    }

    if (num_args > 1)
    {
        LOG_ERROR(L"More than one arg is not supported.");
    }
    else if (num_args == 1)
    {
        String first_arg = String::reference_to_const_wchar_t(args[0]);

        if (!String::is_null_or_empty(first_arg))
            load_path(first_arg);
    }

    return (initialized = true);
}

bool View_Window::shutdown()
{
    safe_release(wic);
    safe_release(d2d1);
    safe_release(dwrite);
    safe_release(current_image_direct2d);
    safe_release(decoder);

    discard_graphics_resources();

    if (kb_accel != 0)
    {
        DestroyAcceleratorTable(kb_accel);
        kb_accel = 0;
    }

    DestroyWindow(hwnd);
    hwnd = 0;

    return true;
}

void View_Window::load_and_apply_settings()
{
    HRESULT hr;
    char* data  = nullptr;
    UINT64 size = 0;

    Temporary_Allocator_Guard g;
    hr = File_System_Utility::read_file_contents(String::reference_to_const_wchar_t(L"D:/sette.txt"), (void**)&data, &size, g_temporary_allocator);
    
    if (FAILED(hr))
    {
        LOG_HRESULT_ERROR(hr, L"Cannot load settings");
        return;
    }

    if (size == 0)
    {
        LOG_ERROR(L"Cannot load settings because settings file is empty.");
        return;
    }

    if (size > static_cast<UINT64>(INT_MAX))
    {
        LOG_ERROR(L"Cannot load settings because file is too big.");
        return;
    }

    String_Builder line{ g_temporary_allocator };
    Line_Reader reader;
    reader.set_source(data, static_cast<int>(size));
    reader.set_string_builder(&line);

    while (reader.next_line())
        debug(line.buffer);
}

String View_Window::get_file_info_absolute_path(const String& folder, const File_Info* file_info, IAllocator* allocator)
{
    const String strings[] ={ folder, file_info->path };
    const size_t num = ARRAYSIZE(strings);

    return String::join(L'/', strings, num, allocator);
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

void View_Window::load_path(const String& file_path)
{
    HRESULT hr = 0;

    String folder_path;
    hr = File_System_Utility::extract_folder_path(file_path, &folder_path);
    if (FAILED(hr)) {
        error_box(hr);
        return;
    }

    Sequence<File_Info> files;
    hr = File_System_Utility::get_folder_files(&files, folder_path, image_filter, nullptr);
    if (FAILED(hr)) {
        error_box(hr);
        return;
    }

    Temporary_Allocator_Guard g;
    String file_name;
    if (!File_System_Utility::extract_file_name_from_path(file_path, &file_name, g_temporary_allocator)) {
        error_box(hr);
        return;
    }

    current_folder = folder_path;
    current_files = files;
    current_file_index = -1;
    
    hr = sort_current_images(Sort_Mode::Date_Created, Sort_Order::Descending);
    if (FAILED(hr)) {
        current_file_index = 0;
        error_box(hr);
        return;
    }

    int index = find_file_info_by_path(file_name);
    if (index == -1)
    {
        LOG_ERROR(L"Unable to find file '%s'\n", file_path.data);
        current_file_index = 0;
    }
    else
    {
        current_file_index = index;
    }

    view_file_index(index);
}

void View_Window::view_prev()
{
    if (current_files.count == 1)
        return;
    if (current_file_index < 0 || current_files.is_empty())
        return;

    int index = current_file_index - 1;
    if (index >= 0)
        view_file_index(index);
    else
        view_file_index(current_files.count + index);
}

void View_Window::view_next()
{
    if (current_files.count == 1)
        return;
    if (current_file_index < 0 || current_files.is_empty())
        return;
    
    int index = current_file_index + 1;
    if (index < current_files.count)
        view_file_index(index);
    else
        view_file_index(current_files.count - index);
}

void View_Window::view_first()
{
    if (current_files.is_empty())
        return;

    view_file_index(0);
}

void View_Window::view_last()
{
    if (current_files.is_empty())
        return;

    view_file_index(current_files.count - 1);
}

void View_Window::view_file_index(int index)
{
    E_VERIFY(index >= 0);

    if (!current_files.is_valid_index(index))
        return;
    
    File_Info* file = &current_files.data[index];

    Temporary_Allocator_Guard g;
    String full_path = get_file_info_absolute_path(current_folder, file, g_temporary_allocator);
    if (String::is_null(full_path))
        __debugbreak();

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr;

    do
    {
        hr = create_decoder_from_file_path(full_path, &decoder);
        if (SUCCEEDED(hr))
        {
            break;
        }
        else
        {
            Temporary_Allocator_Guard g;
            String_Builder sb{ g_temporary_allocator };
            sb.begin();
            sb.append_format(
                L"Cannot load image \"%s\": \"%s\" (HRESULT: %#010x). Retry?",
                full_path.data,
                hresult_to_string(hr),
                hr);
            sb.end();
            // @TODO: bool ignored

            int result;
            hr = TaskDialog(hwnd, 0, L"Error", L"Image loading error", sb.buffer, TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON, TD_ERROR_ICON, &result);
            // @TODO: hresult ignored

            if (result == IDRETRY)
                continue;
            else
                break;
        }
    } while (1);

    if (FAILED(hr) || decoder == nullptr)
        return;

    if (set_current_image(decoder)) {
        current_file_index = index;
        update_view_title();
    }
}

void View_Window::update_view_title()
{
    File_Info* current = get_current_file_info();
    if (current == nullptr)
        return;

    const wchar_t* path = current->path.data;

    String_Builder title{ g_temporary_allocator };
    Temporary_Allocator_Guard g;

    title.begin();
    title.append_format(L"(%i/%i) %s", current_file_index + 1, current_files.count, path);
    if (!title.end())
        LOG_ERROR(L"Unable to update title.\n");
    else 
        SetWindowTextW(hwnd, title.buffer);
}

bool View_Window::set_current_image(IWICBitmapDecoder* bitmap_decoder)
{
    E_VERIFY_NULL_R(bitmap_decoder, false);
    if (!release_current_image())
        return false;

    HRESULT hr;
    decoder = bitmap_decoder;

    IWICBitmapFrameDecode* bitmap_frame = nullptr;
    hr = bitmap_decoder->GetFrame(0, &bitmap_frame);

    WICPixelFormatGUID pixel_format;
    hr = bitmap_frame->GetPixelFormat(&pixel_format);
    if (FAILED(hr)) {
        LOG_HRESULT_ERROR(hr, L"Unable to get pixel format of bitmap frame.\n");
        return false;
    }

    IWICFormatConverter* converter = nullptr;
    IWICBitmapSource* actual_source = bitmap_frame;
    if (pixel_format != GUID_WICPixelFormat32bppPBGRA)
    {
        hr = wic->CreateFormatConverter(&converter);
        if (FAILED(hr)) {
            LOG_HRESULT_ERROR(hr, L"Unable to create format converter.\n");
            return false;
        }

        converter->Initialize(bitmap_frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);
        actual_source = converter;
    }

    hr = hwnd_target->CreateBitmapFromWicBitmap(actual_source, &current_image_direct2d);
    if (FAILED(hr)) {
        LOG_HRESULT_ERROR(hr, L"Unable to create Direct2D bitmap from WIC bitmap.\n");
        return false;
    }

    if (converter != nullptr)
        converter->Release();

    D2D1_SIZE_F image_size = current_image_direct2d->GetSize();
    current_image_size = image_size;

    set_desired_client_size((int)image_size.width, (int)image_size.height);

    InvalidateRect(hwnd, nullptr, true);
    bitmap_frame->Release();

    return true;
}

bool View_Window::get_client_area(int* width, int* height)
{
    E_VERIFY_NULL_R(width, false);
    E_VERIFY_NULL_R(height, false);
    
    RECT rect;
    if (!GetClientRect(hwnd, &rect))
        return false;

    *width  = rect.right  - rect.left;
    *height = rect.bottom - rect.top;

    return true;
}

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

void View_Window::handle_change_display_mode_action()
{
    HRESULT hr;

    switch (display_mode)
    {
        case Display_Mode::Windowed:
        {
            hr = set_display_mode(Display_Mode::Fullscreen);
            if (SUCCEEDED(hr))
            {
                BOOL result = ModifyMenuW(
                    view_menu,
                    (UINT_PTR)View_Menu_Item::Change_Display_Mode,
                    MF_BYCOMMAND | MF_STRING,
                    (UINT_PTR)View_Menu_Item::Change_Display_Mode,
                    L"Exit fullscreen");

                if (!result)
                    LOG_LAST_WIN32_ERROR(L"Unable to modify view menu");
            }
            break;
        }

        case Display_Mode::Fullscreen:
        {
            hr = set_display_mode(Display_Mode::Windowed);
            if (SUCCEEDED(hr))
            {
                BOOL result = ModifyMenuW(
                    view_menu,
                    (UINT_PTR)View_Menu_Item::Change_Display_Mode,
                    MF_BYCOMMAND | MF_STRING,
                    (UINT_PTR)View_Menu_Item::Change_Display_Mode,
                    L"Enter fullscreen");

                if (!result)
                    LOG_LAST_WIN32_ERROR(L"Unable to modify view menu");

                File_Info* current = get_current_file_info();

                if (current != nullptr)
                {
                    D2D1_SIZE_F image_size = current_image_direct2d->GetSize();
                    set_desired_client_size(
                        static_cast<int>(image_size.width),
                        static_cast<int>(image_size.height)
                    );
                }
            }
            break;
        }

        default:
        {
            LOG_ERROR(L"Unknown display mode %d", static_cast<int>(display_mode));
            return;
        }
    }

    if (FAILED(hr))
        LOG_HRESULT_ERROR(hr, L"Unable to change display mode to %d", static_cast<int>(display_mode));
}

void View_Window::handle_copy_filename_to_clipboard_menu_item()
{
    File_Info* current = get_current_file_info();
    if (current == nullptr)
    {
        MessageBoxW(hwnd, L"No image is loaded.", L"Error", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const String& filename = current->path;
    if (String::is_null_or_empty(filename))
    {
        MessageBoxW(hwnd, L"Filename is invalid.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    BOOL result;
    result = OpenClipboard(hwnd);
    if (!result)
    {
        LOG_LAST_WIN32_ERROR(L"Cannot open clipboard");
        MessageBoxW(hwnd, L"Cannot open clipboard", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    EmptyClipboard();
    
    defer (CloseClipboard());

    HGLOBAL clip_store = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (filename.count + 1));
    if (clip_store == nullptr)
    {
        LOG_LAST_WIN32_ERROR(L"Cannot allocate enough memory to save a string");
        return;
    }

    wchar_t* clip_data = (wchar_t*)GlobalLock(clip_store);
    if (clip_data == nullptr)
    {
        LOG_LAST_WIN32_ERROR(L"Unable to lock global memory");
        return;
    }
    defer (GlobalUnlock(clip_store));
    
    wmemcpy(clip_data, filename.data, filename.count);
    clip_data[filename.count] = L'\0';

    result = GlobalUnlock(clip_store);
    DWORD last_error = GetLastError();
    if (last_error != NO_ERROR)
    {
        LOG_WIN32_ERROR(last_error, L"Unable to unlock global memory");
        return;
    }

    if (0 == SetClipboardData(CF_UNICODETEXT, clip_store))
    {
        LOG_LAST_WIN32_ERROR(L"Unable to set clipboard data");
        return;
    }
}

int View_Window::find_file_info_by_path(const String& path)
{
    if (String::is_null_or_empty(path))
        return -1;

    for (int i = 0; i < current_files.count; ++i)
    {
        const File_Info& file_info = current_files.data[i];

        if (String::equals(path, file_info.path))
            return i;
    }

    return -1;
}

HRESULT View_Window::set_scaling_mode(Scaling_Mode mode, float scaling)
{
    if (mode == Scaling_Mode::No_Scaling)
    {
        this->scaling_mode = mode;
        this->scaling = 1.0f;

        InvalidateRect(hwnd, nullptr, false);

        return S_OK;
    }

    if (mode == Scaling_Mode::Percentage)
    {
        E_VERIFY_R(scaling > 0.0f, E_INVALIDARG);

        this->scaling_mode = mode;
        this->scaling = scaling;

        InvalidateRect(hwnd, nullptr, false);

        return S_OK;
    }

    if (mode == Scaling_Mode::Fit_To_Window)
    {
        this->scaling_mode = mode;
        this->scaling = 1.0f;

        InvalidateRect(hwnd, nullptr, false);

        return S_OK;
    }

    // Unknown mode.
    E_VERIFY_R(false, E_UNEXPECTED);
}

HRESULT View_Window::set_display_mode(Display_Mode mode)
{
    if (this->display_mode == mode)
        return S_OK;

    if (mode == Display_Mode::Fullscreen)
    {
        SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG>(fullscreen_display_mode_flags));
        SetWindowPos(hwnd, nullptr, 0, 0, desktop_width, desktop_height, SWP_FRAMECHANGED);
        ShowWindow(hwnd, SW_SHOW);
    
        this->display_mode = mode;
    
        return S_OK;
    }
    else if (mode == Display_Mode::Windowed)
    {
        SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG>(windowed_display_mode_flags));

        // @TODO: use 'set_desired_size'
        SetWindowPos(hwnd, nullptr, 0, 0, desktop_work_width, desktop_work_height, SWP_FRAMECHANGED);
        ShowWindow(hwnd, SW_SHOW);

        this->display_mode = mode;

        return S_OK;
    }

    LOG_ERROR(L"Unknown display mode %d", static_cast<int>(mode));
    return E_INVALIDARG;
}

HRESULT View_Window::draw_current_image()
{
    int client_width, client_height;
    get_client_area(&client_width, &client_height);

    D2D1_SIZE_F image_size = current_image_size;

    if (scaling_mode == Scaling_Mode::Fit_To_Window)
    {
        fit_to_window:
        if (image_size.width > client_width)
        {
            float aspect = client_width / image_size.width;

            image_size.width  *= aspect;
            image_size.height *= aspect;
        }

        if (image_size.height > client_height)
        {
            float aspect = client_height / image_size.height;

            image_size.width  *= aspect;
            image_size.height *= aspect;
        }
    }
    else if (scaling_mode == Scaling_Mode::Percentage)
    {
        image_size.width *= scaling;
        image_size.height *= scaling;
    }
    else if (scaling_mode == Scaling_Mode::No_Scaling)
    {
        // Don't do anything.
    }
    else
    {
        LOG_ERROR(L"Unknown scaling mode %d.", scaling_mode);
        set_scaling_mode(Scaling_Mode::Fit_To_Window, 1.0f);
        goto fit_to_window;
    }

    float img_hw = (int)image_size.width  / 2.0f;
    float img_hh = (int)image_size.height / 2.0f;
    float cli_hw = client_width  / 2.0f;
    float cli_hh = client_height / 2.0f;

    D2D1_RECT_F dest_rect = D2D1::RectF(
        ceilf(cli_hw - img_hw),
        ceilf(cli_hh - img_hh),
        ceilf(cli_hw + img_hw),
        ceilf(cli_hh + img_hh)
    );

    hwnd_target->DrawBitmap(current_image_direct2d, dest_rect);
    draw_current_image_info();

    return S_OK;
}

HRESULT View_Window::draw_placeholder()
{
    default_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    default_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    hwnd_target->DrawTextW(L"Please load image", (int)wcslen(L"Please load image"), default_text_format,
        client_area_as_rectf(), default_text_foreground_brush);

    return S_OK;
}

#pragma region Sort functions
typedef int(__cdecl *QSort_Func)(const void* a, const void* b);

int sort_by_name_asc(const File_Info* a, const File_Info* b) {
    return wcsncmp(a->path.data, b->path.data, max(a->path.count, b->path.count));
}
int sort_by_name_desc(const File_Info* a, const File_Info* b) {
    return -1 * wcsncmp(a->path.data, b->path.data, max(a->path.count, b->path.count));
}
int sort_by_date_created_asc(const File_Info* a, const File_Info* b) {
    return CompareFileTime(&a->date_created, &b->date_created);
}
int sort_by_date_created_desc(const File_Info* a, const File_Info* b) {
    return -1 * CompareFileTime(&a->date_created, &b->date_created);
}
int sort_by_date_accessed_asc(const File_Info* a, const File_Info* b) {
    return CompareFileTime(&a->date_accessed, &b->date_accessed);
}
int sort_by_date_accessed_desc(const File_Info* a, const File_Info* b) {
    return -1 * CompareFileTime(&a->date_accessed, &b->date_accessed);
}
int sort_by_date_modified_asc(const File_Info* a, const File_Info* b) {
    return CompareFileTime(&a->date_modified, &b->date_modified);
}
int sort_by_date_modified_desc(const File_Info* a, const File_Info* b) {
    return -1 * CompareFileTime(&a->date_modified, &b->date_modified);
}

// These functions MUST be in order as in Sort_Mode enum.
// Ascending first, descending second.
static QSort_Func sort_funcs[]
{
    (QSort_Func)sort_by_name_asc,
    (QSort_Func)sort_by_date_created_asc,
    (QSort_Func)sort_by_date_accessed_asc,
    (QSort_Func)sort_by_date_modified_asc,

    (QSort_Func)sort_by_name_desc,
    (QSort_Func)sort_by_date_created_desc,
    (QSort_Func)sort_by_date_accessed_desc,
    (QSort_Func)sort_by_date_modified_desc,
};
#pragma endregion

HRESULT View_Window::sort_current_images(Sort_Mode mode, Sort_Order order)
{
    E_VERIFY_R(mode >= Sort_Mode::Name || mode <= Sort_Mode::Date_Modified, E_INVALIDARG);
    E_VERIFY_R(order >= Sort_Order::Ascending || order <= Sort_Order::Descending, E_INVALIDARG);

    if (current_files.data == nullptr || current_files.count <= 1)
        return S_OK;

    File_Info* current = get_current_file_info();
    QSort_Func sort_func = sort_funcs[((int)order * (int)Sort_Mode::NUM_MODES) + (int)mode];

    if (sort_func != nullptr)
        qsort(current_files.data, current_files.count, sizeof(current_files.data[0]), sort_func);

    if (current != nullptr)
    {
        for (int i = 0; i < current_files.count; ++i)
        {
            if (&current_files.data[i] == current)
            {
                current_file_index = i;
                break;
            }
        }
    }

    //sort_mode = mode;
    //sort_order = order;

    return S_OK;
}

HRESULT View_Window::create_decoder_from_file_path(const String& file_path, IWICBitmapDecoder** decoder)
{
    E_VERIFY_R(!String::is_null_or_empty(file_path), E_INVALIDARG);
    E_VERIFY_NULL_R(decoder, E_INVALIDARG);

    HRESULT hr;
    IStream* stream = nullptr;
    hr = SHCreateStreamOnFileEx(file_path.data, STGM_READ, FILE_ATTRIBUTE_NORMAL, false, nullptr, &stream);
    if (FAILED(hr))
        return hr;
    defer (safe_release(stream));

    hr = wic->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, decoder);
    return hr;
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
        
        if (!TranslateAcceleratorW(hwnd, kb_accel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return msg.message == WM_QUIT ? (int)msg.wParam : 0;
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

            hwnd_target->Resize(D2D1::SizeU(new_width, new_height));

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
            draw_window();
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
                case View_Menu_Item::Change_Display_Mode:
                    handle_change_display_mode_action();
                    break;
                case View_Menu_Item::Copy_Filename_To_Clipboard:
                    handle_copy_filename_to_clipboard_menu_item();
                    break;
                default:
                   E_DEBUGBREAK(); // Unknown item
            }

            return 0;
        }
        case WM_ERASEBKGND:
        {
            return 0;
        }
        case (UINT)View_Window_Message::Show_After_Entering_Event_Loop:
        {
            draw_window();
            ShowWindow(hwnd, SW_SHOWNORMAL);
            return 0;
        }
        case WM_COMMAND:
        {
            bool is_accelerator = HIWORD(wParam) == 1;
            if (!is_accelerator)
                return 0;

            View_Shortcut key = (View_Shortcut)LOWORD(wParam);
            switch (key)
            {
                case View_Shortcut::View_Prev:
                    view_prev();
                    break;
                case View_Shortcut::View_Next:
                    view_next();
                    break;
                case View_Shortcut::View_First:
                    view_first();
                    break;
                case View_Shortcut::View_Last:
                    view_last();
                    break;
                case View_Shortcut::View_Show_File_In_Explorer:
                {
                    File_Info* current = get_current_file_info();
                    if (current != nullptr)
                    {
                        Temporary_Allocator_Guard g;
                        String abs_path = get_file_info_absolute_path(current_folder, current, g_temporary_allocator);
                        if (!String::is_null(abs_path) && SUCCEEDED(File_System_Utility::normalize_path(abs_path)))
                            File_System_Utility::select_file_in_explorer(abs_path);
                    }
                    break;
                }
                case View_Shortcut::Fast_Quit:
                {
                    shutdown();
                    break;
                }
                case View_Shortcut::Change_Display_Mode:
                {
                    handle_change_display_mode_action();
                    break;
                }
            }

            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HRESULT View_Window::draw_current_image_info()
{
    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    D2D1_SIZE_F size = current_image_direct2d->GetSize();

    sb.begin();
    sb.append_format(L"%dx%d", (int)size.width, (int)size.height);
    if (!sb.end())
        return E_OUTOFMEMORY;

    // @TODO: read from settings
    const float shadow_offset = 1.1f;
    const float margin_left = 4.0f;
    const float margin_top = 4.0f;

    D2D1_RECT_F area = client_area_as_rectf();
    area.left += margin_left + shadow_offset;
    area.top += margin_top + shadow_offset;

    hwnd_target->DrawTextW(sb.buffer, sb.count, image_info_text_format, area, image_info_text_shadow_brush);

    area.left -= shadow_offset;
    area.top -= shadow_offset;

    hwnd_target->DrawTextW(sb.buffer, sb.count, image_info_text_format, area, image_info_text_brush);
    
    return S_OK;
}

D2D1_RECT_F View_Window::client_area_as_rectf()
{
    RECT client_area;
    GetClientRect(hwnd, &client_area);

    return D2D1::RectF(
        0.0f,
        0.0f,
        static_cast<float>(client_area.right),
        static_cast<float>(client_area.bottom));
}

void View_Window::set_desired_client_size(int desired_width, int desired_height)
{
    E_VERIFY(desired_width > 0);
    E_VERIFY(desired_height > 0);

    HRESULT hr;

    if (this->display_mode == Display_Mode::Fullscreen)
        return;

    WINDOWPLACEMENT window_info;
    window_info.length = sizeof(window_info);
    if (GetWindowPlacement(hwnd, &window_info))
    {
        if (window_info.showCmd == SW_MAXIMIZE)
            return; // Window is maximized, don't adjust window size.
    }

    int window_width  = desired_width;
    int window_height = desired_height;
    
    hr = adjust_window_rect(&window_width, &window_height, GetWindowLongW(hwnd, GWL_STYLE), GetWindowLongW(hwnd, GWL_EXSTYLE));
    if (FAILED(hr))
        return;

    // @TODO: on Windows 10 horizontal borders are 1 px from each side, but
    // border_horiz is 16 pixels. Also we have 7px gap between taskbar and
    // window vertically.
    //int border_horiz = window_width  - desired_width;
    //int border_verti = window_height - desired_height;

    // Make sure we don't have window out of desktop bounds.
    if (window_width > desktop_work_width)
    {
        float aspect = (float)desktop_work_width / window_width;

        window_width  = static_cast<int>(aspect * window_width);
        window_height = static_cast<int>(aspect * window_height);
    }

    if (window_height > desktop_work_height)
    {
        float aspect = (float)desktop_work_height / window_height;

        window_width  = static_cast<int>(aspect * window_width);
        window_height = static_cast<int>(aspect * window_height);
    }

    // Calculate window center. Use work size to not clutter the taskbar.
    int window_x = (desktop_work_width / 2)  - (window_width / 2);
    int window_y = (desktop_work_height / 2) - (window_height / 2);

    BOOL result;
    result = SetForegroundWindow(hwnd);
    if (!result)
        LOG_LAST_WIN32_ERROR(L"Cannot bring view window to foreground");

    result = SetWindowPos(
        hwnd,
        0,
        window_x,
        window_y,
        window_width,
        window_height,
        SWP_NOCOPYBITS | SWP_NOZORDER | SWP_NOOWNERZORDER);

    if (!result)
        LOG_LAST_WIN32_ERROR(L"Cannot set view window position");
}

void View_Window::error_box(HRESULT hr)
{
    String_Builder sb{ g_temporary_allocator };
    Temporary_Allocator_Guard g;

    sb.begin();
    sb.append_string(L"Error: ");
    sb.append_string(hresult_to_string(hr));
    sb.append_format(L"\n\nHRESULT: %#010x", hr);
    if (!sb.end())
        MessageBoxW(0, L"Got an error, but cannot format it.", L"Error", MB_OK | MB_ICONERROR);
    else
        MessageBoxW(0, sb.buffer, L"Error", MB_OK | MB_ICONERROR);
}

void View_Window::error_box(const wchar_t* message)
{   
    String_Builder sb{ g_temporary_allocator };
    Temporary_Allocator_Guard g;

    sb.begin();
    sb.append_string(L"Error: ");
    sb.append_string(message);
    if (!sb.end())
        MessageBoxW(hwnd, L"Got an error, but cannot format it.", L"Error", MB_OK | MB_ICONERROR);
    else
        MessageBoxW(hwnd, sb.buffer, L"Error", MB_OK | MB_ICONERROR);
}

File_Info* View_Window::get_current_file_info() const
{
    if (!current_files.is_valid_index(current_file_index))
        return nullptr;

    return &current_files.data[current_file_index];
}

void View_Window::draw_window()
{
    E_VERIFY_NULL(hwnd_target);
    E_VERIFY_NULL(default_text_format);
    E_VERIFY_NULL(default_text_foreground_brush);

    int call_num = 0;
start:
    HRESULT hr = S_OK;

    hwnd_target->BeginDraw();
    hwnd_target->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    if (current_image_direct2d == nullptr)
    {
        draw_placeholder();
    }
    else
    {
        draw_current_image();
    }

    hr = hwnd_target->EndDraw();
    if (SUCCEEDED(hr))
    {
        ValidateRect(hwnd, nullptr);
    }
    else
    {
        if (call_num >= 3)
        {
            LOG_ERROR(L"Unable to recover " __FUNCTIONW__);
            return;
        }

        if (hr == D2DERR_RECREATE_TARGET)
        {
            discard_graphics_resources();
            hr = create_graphics_resources();
            if (FAILED(hr))
            {
                error_box(hr);
                return;
            }
            else
            {
                ++call_num;
                goto start;
                return;
            }
        }
        else
        {
            error_box(hr);
            return;
        }
    }
}

HRESULT View_Window::create_graphics_resources()
{
    HRESULT hr = S_OK;

    // Create render target
    hr = Graphics_Utility::create_hwnd_render_target(hwnd, &hwnd_target);
    if (FAILED(hr))
        goto fail;

    // Create default text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI", // @TODO: hardcoded
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        32.0f,
        L"",
        &default_text_format);
    if (FAILED(hr))
        goto fail;

    // Create default text brush
    hr = hwnd_target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &default_text_foreground_brush);
    if (FAILED(hr))
        goto fail;

    // Create image info text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        20.0f,
        L"",
        &image_info_text_format);
    if (FAILED(hr))
        goto fail;

    // Create image info text brush
    hr = hwnd_target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &image_info_text_brush);
    if (FAILED(hr))
        goto fail;

    // Create image info text shadow brush
    hr = hwnd_target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &image_info_text_shadow_brush);
    if (FAILED(hr))
        goto fail;

    return S_OK;

fail:
    discard_graphics_resources();
    return hr;
}

void View_Window::discard_graphics_resources()
{
    safe_release(image_info_text_format);
    safe_release(image_info_text_brush);
    safe_release(image_info_text_shadow_brush);
    safe_release(default_text_foreground_brush);
    safe_release(default_text_format);
    safe_release(hwnd_target);
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

HRESULT adjust_window_rect(int* width, int* height, DWORD window_flags, DWORD window_flags_ex)
{
    E_VERIFY_NULL_R(width,  E_INVALIDARG);
    E_VERIFY_NULL_R(height, E_INVALIDARG);

    RECT size = { 0 };
    size.right  = *width;
    size.bottom = *height;

    if (!AdjustWindowRectEx(&size, window_flags, false, window_flags_ex))
    {
        LOG_LAST_WIN32_ERROR(L"Unable to adjust window rectangle.");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *width  = size.right  - size.left;
    *height = size.bottom - size.top;
    
    return S_OK;
}
