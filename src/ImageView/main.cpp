#include <Windows.h>
#include <Commctrl.h>

#include "view_window.hpp"
#include "string_builder.hpp"
#include "graphics_utility.hpp"
#include "file_system_utility.hpp"
#include "windows_utility.hpp"
#include "line_reader.hpp"
#include "pool_allocator.hpp"

#pragma comment(lib, "Comctl32.lib")

void display_error_box(const wchar_t* message);
void display_error_box_format(const wchar_t* backup_message, const wchar_t* format, ...);
void display_error_box_hresult(const wchar_t* backup_message, HRESULT hresult, const wchar_t* format, ...);


int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    HeapSetInformation(0, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif

    if (!g_temporary_allocator->set_size(32 * 1024))
    {
        display_error_box(L"Unable to initialize temporary allocator.");
        return -1;
    }

    HRESULT hr;
    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
    if (FAILED(hr))
    {
        display_error_box_format(
            L"Unable to initialize COM library.",

            L"Unable to initialize COM library: \"%s\""
            L"\n\n"
            L"HRESULT: %#010x",
            hresult_to_string(hr),
            hr);
        
        return -1;
    }

    hr = OleInitialize(nullptr);
    if (FAILED(hr))
    {
        display_error_box_format(
            L"Unable to initialize OLE library.",

            L"Unable to initialize OLE library: \"%s\""
            L"\n\n"
            L"HRESULT: %#010x",
            hresult_to_string(hr),
            hr);

        return -1;
    }

    const wchar_t* graphics_error = nullptr;
    hr = Graphics_Utility::initialize(&graphics_error);
    if (FAILED(hr))
    {
        display_error_box_hresult(
            graphics_error,
            hr,
            L"%s",
            graphics_error);

        return -1;
    }

    View_Window_Init_Params params;
    params.hInstance = hInstance;
    params.lpCmdLine = lpCmdLine;
    params.nCmdShow  = nCmdShow;

    params.wic = Graphics_Utility::wic;
    params.d2d1 = Graphics_Utility::d2d1;
    params.dwrite = Graphics_Utility::dwrite;

    params.show_after_entered_event_loop = true;

    String command_line{ lpCmdLine, static_cast<int>(wcslen(lpCmdLine)) };

    View_Window view;
    if (!view.initialize(params, command_line)) {
        // report_error(L"Unable to initialize main window.\n");
        return 1;
    }

    int return_code = view.enter_message_loop();

    view.shutdown();
    Graphics_Utility::shutdown();

    return return_code;
}

void display_error_box(const wchar_t* message)
{
    E_VERIFY_NULL(message);

    MessageBoxW(0, message, L"Image View: initialization error", MB_OK | MB_ICONERROR);
}

void display_error_box_format(const wchar_t* backup_message, const wchar_t* format, ...)
{
    E_VERIFY_NULL(backup_message);
    E_VERIFY_NULL(format);

    const wchar_t* message = backup_message;

    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    va_list args;
    va_start(args, format);

    sb.begin();
    sb.append_format(format, args);
    if (sb.end())
        message = sb.buffer;

    va_end(args);

    display_error_box(message);
}

void display_error_box_hresult(const wchar_t* backup_message, HRESULT hresult, const wchar_t* format, ...)
{
    E_VERIFY_NULL(backup_message);
    E_VERIFY_NULL(format);

    const wchar_t* message = backup_message;

    Temporary_Allocator_Guard g;
    String_Builder sb{ g_temporary_allocator };

    va_list args;
    va_start(args, format);

    sb.begin();
    sb.append_format(format, args);
    sb.append_string(L": \"");
    sb.append_string(hresult_to_string(hresult));
    sb.append_string(L"\".");
    sb.append_string(L"\n\n");
    sb.append_format(L"HRESULT: %#010x", hresult);
    if (sb.end())
        message = sb.buffer;

    va_end(args);

    display_error_box(message);
}
