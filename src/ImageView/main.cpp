#include "view_window.hpp"
#include "graphics_utility.hpp"
#include "file_system_utility.hpp"

bool filter(const WIN32_FIND_DATA& data, void* userdata)
{
    return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    HeapSetInformation(0, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif

    if (!g_temporary_allocator->set_size(32 * 1024)) {
        report_error(L"Unable to initialize temporary allocator.\n");
        return -1;
    }

    if (FAILED(CoInitialize(nullptr))) {
        report_error(L"Unable to initialize COM.\n");
        return -1;
    }

    if (!Graphics_Utility::initialize()) {
        report_error(L"Unable to initialize graphics.\n");
        return -1;
    }

    View_Window_Init_Params params;
    params.hInstance = hInstance;
    params.lpCmdLine = lpCmdLine;
    params.nCmdShow  = nCmdShow;

    params.wic = Graphics_Utility::wic;
    params.d2d1 = Graphics_Utility::d2d1;
    params.dwrite = Graphics_Utility::dwrite;

    params.show_after_entering_event_loop = true;

    View_Window view;
    if (!view.initialize(params)) {
        report_error(L"Unable to initialize main window.\n");
        return 1;
    }

    int return_code = view.enter_message_loop();

    view.shutdown();
    Graphics_Utility::shutdown();

    return return_code;
}