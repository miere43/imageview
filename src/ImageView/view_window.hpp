#pragma once
#include <Windows.h>
#include <wincodec.h>
#include <Shobjidl.h>

#include "graphics_utility.hpp"

struct View_Window_Init_Params
{
    // Forwarded from wWinMain
    HINSTANCE hInstance = 0;
    LPCWSTR lpCmdLine   = nullptr;
    int nCmdShow = SW_SHOW;

    ID2D1Factory1* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    IWICImagingFactory* wic = nullptr;

    // -1 means center it!
    int window_x = -1;
    int window_y = -1;
    int window_client_area_width  = 400;
    int window_client_area_height = 400;

    bool show_after_entering_event_loop = false;
};

struct Image_Info
{
    wchar_t* path;
};

struct View_Window
{
    enum View_Window_State
    {
        VWS_Default = 0,
        VWS_Viewing_Image = 1
    };

    HWND hwnd = 0;
    bool run_message_loop = false;
    bool initialized = false;

    View_Window_State state = VWS_Default;

    // Graphics resources
    ID2D1Factory1* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    IWICImagingFactory* wic = nullptr;
    ID2D1HwndRenderTarget* g = nullptr;

    IDWriteTextFormat* default_text_format = nullptr;
    ID2D1SolidColorBrush* default_text_foreground_brush = nullptr;

    ID2D1Bitmap* current_image_direct2d = nullptr;
    IWICBitmapSource* current_image_wic = nullptr;

    // Menus
    HMENU view_menu = 0;

    bool initialize(const View_Window_Init_Params& params);
    bool shutdown();

    bool set_from_file_path(const wchar_t* file_path);
    
    bool set_current_image(IWICBitmapSource* image);
    bool set_file_name(const wchar_t* file_name, size_t length);

    bool get_client_area(int* width, int* height);
    //bool close_current_image();

    bool release_current_image();
    bool handle_open_file_action();

    // @TODO: should be WIC method
    IWICBitmapSource* load_image_from_path(const wchar_t* path);

    int enter_message_loop();
    LRESULT __stdcall wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    //void set_to_default_state(View_Window_State previous_state);
    //void go_to_state(View_Window_State new_state);
    D2D1_RECT_F client_area_as_rectf();
};