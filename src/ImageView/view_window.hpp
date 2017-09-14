#pragma once
#include <Windows.h>
#include <wincodec.h>
#include <Shobjidl.h>

#include "file_system_utility.hpp"
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

struct View_Window
{
    enum View_Window_State
    {
        VWS_Default = 0,
        VWS_Viewing_Image = 1
    };

    const static int Center_Window = 1;
    const static int Maximize_If_Too_Big = 2;

    HWND hwnd = 0;
    bool run_message_loop = false;
    bool initialized = false;

    String current_folder;
    Sequence<File_Info> current_files;
    int current_file_index = -1;
    IWICBitmapDecoder* current_file_decoder = nullptr;
    
    View_Window_State state = VWS_Default;

    // Graphics resources
    ID2D1Factory1* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    IWICImagingFactory* wic = nullptr;
    ID2D1HwndRenderTarget* g = nullptr;

    IDWriteTextFormat* default_text_format = nullptr;
    ID2D1SolidColorBrush* default_text_foreground_brush = nullptr;

    ID2D1Bitmap* current_image_direct2d = nullptr;
    IWICBitmapDecoder* decoder = nullptr;

    //
    int desktop_width = 800;
    int desktop_height = 600;
    int desktop_work_width = 800;
    int desktop_work_height = 600;

    // Menus
    HMENU view_menu = 0;

    bool initialize(const View_Window_Init_Params& params);
    bool shutdown();

    void load_path(const String& file_path);
    void view_prev();
    void view_next();
    void view_file_index(int index);

    void update_view_title();

    String get_file_info_absolute_path(const String& folder, const File_Info* file_info, IAllocator* allocator);

    bool set_current_image(IWICBitmapDecoder* image);
    bool get_client_area(int* width, int* height);
    bool release_current_image();
    
    bool handle_open_file_action();
    int find_file_info_by_path(const String& path);

    IWICBitmapDecoder* create_decoder_from_file_path(const wchar_t* path);

    int enter_message_loop();
    LRESULT __stdcall wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
    wchar_t title_buffer[512] = { 0 };

    //void set_to_default_state(View_Window_State previous_state);
    //void go_to_state(View_Window_State new_state);
    D2D1_RECT_F client_area_as_rectf();
    void resize_window(int new_width, int new_height, int flags);
};