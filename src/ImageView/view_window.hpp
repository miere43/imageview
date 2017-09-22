#pragma once
#include <Windows.h>
#include <wincodec.h>
#include <Shobjidl.h>

#include "file_system_utility.hpp"
#include "graphics_utility.hpp"
#include "view_window_drop_target.hpp"


struct Image_Data_Source
{
    enum Data_Source_Type
    {
        Unknown = 0,
        Folder,
        User,
    };

    Data_Source_Type type;
    union
    {
        struct
        {
            String folder_path;
        } f;
    };
};

struct Image_Info
{
    Image_Data_Source* source = nullptr;
    String path;
};

struct View_Window_Init_Params
{
    // Forwarded from wWinMain
    HINSTANCE hInstance = 0;
    LPCWSTR lpCmdLine = nullptr;
    int nCmdShow = SW_SHOW;

    ID2D1Factory1* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    IWICImagingFactory* wic = nullptr;

    // -1 means center it!
    int window_x = -1;
    int window_y = -1;
    int window_client_area_width  = 400;
    int window_client_area_height = 400;

    bool show_after_entered_event_loop = false;
};

// Don't change enum values! Used in View_Window::sort_current_images
// When adding another one, fix View_Window::sort_current_images.
enum class Sort_Mode
{
    Name = 0,
    Date_Created = 1,
    Date_Accessed = 2,
    Date_Modified = 3,
    NUM_MODES
};

enum class Scaling_Mode
{
    // Don't scale image.
    No_Scaling = 0,
    // Resize image to percentage value.
    Percentage = 1,
    // Adjust image to fit the window, but don't resize
    // image over 100%.
    Fit_To_Window = 2,
};

enum class Display_Mode
{
    // Display image in a window.
    Windowed = 0,
    // Display image in borderless fullscreen window.
    Fullscreen = 1
};

// Don't change enum values! Used in View_Window::sort_current_images
enum class Sort_Order
{
    Ascending = 0,
    Descending = 1,
};

struct View_Window
{
    enum View_Window_State
    {
        VWS_Default = 0,
        VWS_Viewing_Image = 1
    };

    HWND hwnd = 0;
    bool initialized = false;
    bool show_image_info = true;

    HACCEL kb_accel = 0;

    String current_folder;
    Sequence<File_Info> current_files;
    int current_file_index = -1;
    IWICBitmapDecoder* current_file_decoder = nullptr;
    
    View_Window_State state = VWS_Default;

    ID2D1Factory1* d2d1 = nullptr;
    IDWriteFactory* dwrite = nullptr;
    IWICImagingFactory* wic = nullptr;

    // Graphics resources
    ID2D1HwndRenderTarget* hwnd_target = nullptr;
    IDWriteTextFormat* default_text_format = nullptr;
    ID2D1SolidColorBrush* default_text_foreground_brush = nullptr;
    IDWriteTextFormat* image_info_text_format = nullptr;
    ID2D1SolidColorBrush* image_info_text_brush = nullptr;
    ID2D1SolidColorBrush* image_info_text_shadow_brush = nullptr;

    D2D1_SIZE_F current_image_size;
    ID2D1Bitmap* current_image_direct2d = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    
    //Sort_Mode sort_mode = Sort_Mode::Undefined;
    //Sort_Order sort_order = Sort_Order::Undefined;

    // Scaling
    Scaling_Mode scaling_mode = Scaling_Mode::No_Scaling;
    float scaling = 1.0f;

    // Display mode
    Display_Mode display_mode = Display_Mode::Windowed;

    //
    int desktop_width = 800;
    int desktop_height = 600;
    int desktop_work_width = 800;
    int desktop_work_height = 600;

    // Menus
    HMENU view_menu = 0;

    bool initialize(const View_Window_Init_Params& params, String command_line);
    bool shutdown();

    void load_and_apply_settings();

    void load_path(const String& file_path);
    
    void view_prev();
    void view_next();
    void view_first();
    void view_last();
    void view_file_index(int index);

    void update_view_title();

    String get_file_info_absolute_path(const String& folder, const File_Info* file_info, IAllocator* allocator);

    bool set_current_image(IWICBitmapDecoder* image);
    bool get_client_area(int* width, int* height);
    bool release_current_image();
    
    // Menus
    bool handle_open_file_action();
    void handle_show_image_info_action();
    void handle_change_display_mode_action();
    void handle_copy_filename_to_clipboard_menu_item();

    int  find_file_info_by_path(const String& path);

    // Scaling
    HRESULT set_scaling_mode(Scaling_Mode mode, float scaling);

    // Display mode
    HRESULT set_display_mode(Display_Mode mode);
    
    // Draw states
    HRESULT draw_current_image();
    HRESULT draw_placeholder();
    HRESULT draw_current_image_info();

    HRESULT sort_current_images(Sort_Mode mode, Sort_Order order);
    HRESULT create_decoder_from_file_path(const String& file_path, IWICBitmapDecoder** decoder);

    int enter_message_loop();
    LRESULT __stdcall wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    //void set_to_default_state(View_Window_State previous_state);
    //void go_to_state(View_Window_State new_state);
    D2D1_RECT_F client_area_as_rectf();

    void set_desired_client_size(int desired_width, int desired_height);

    void error_box(HRESULT hr);
    void error_box(const wchar_t* message);

    File_Info* get_current_file_info() const;
    void draw_window();

    HRESULT create_graphics_resources();
    void discard_graphics_resources();
};