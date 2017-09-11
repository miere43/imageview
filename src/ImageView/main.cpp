#include "view_window.hpp"
#include "graphics_utility.hpp"

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    HeapSetInformation(0, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif

    if (FAILED(CoInitialize(nullptr)))
        return 1;

    if (!Graphics_Utility::initialize())
        return 1;

    View_Window_Init_Params params;
    params.hInstance = hInstance;
    params.lpCmdLine = lpCmdLine;
    params.nCmdShow  = nCmdShow;

    params.wic = Graphics_Utility::wic;
    params.d2d1 = Graphics_Utility::d2d1;
    params.dwrite = Graphics_Utility::dwrite;

    params.show_after_entering_event_loop = true;

    View_Window view;
    if (!view.initialize(params))
        return 1;

    //view.set_file_name(L"lol", 3);
    //view.set_current_image(frame);

    int return_code = view.enter_message_loop();

    view.shutdown();
    Graphics_Utility::shutdown();

    return return_code;
}