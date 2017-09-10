#include "view_window.hpp"


/*
    
    TODO:
    - be DPI aware
    - switch to spaces

*/

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    HeapSetInformation(0, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif

    if (FAILED(CoInitialize(nullptr)))
        return 0;

    HRESULT hr;

    IWICImagingFactory* wic_factory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic_factory));
    if (FAILED(hr))
        return false;

    View_Window_Init_Params params;
    params.hInstance = hInstance;
    params.lpCmdLine = lpCmdLine;
    params.nCmdShow  = nCmdShow;

    params.wic_factory = wic_factory;
    params.show_after_entering_event_loop = true;

    View_Window view;
    if (!view.initialize(params))
        return 1;

    //view.set_file_name(L"lol", 3);
    //view.set_current_image(frame);

    int return_code = view.enter_message_loop();
    view.discard();

    return return_code;
}