#pragma once
#include <Windows.h>
#include <wincodec.h>
#include "painter.hpp"

struct View_Window_Init_Params
{
	// Forwarded from wWinMain
	HINSTANCE hInstance = 0;
	LPCWSTR lpCmdLine   = nullptr;
	int nCmdShow = SW_SHOW;

	IWICImagingFactory* wic_factory = nullptr;

	// -1 means center it!
	int window_x = -1;
	int window_y = -1;
	int window_client_area_width  = 400;
	int window_client_area_height = 400;

	bool show_after_entering_event_loop = false;
};

struct View_Window
{
	HWND hwnd = 0;
	bool run_message_loop = false;
	bool initialized = false;

	Direct2D direct2d;
	IWICImagingFactory* wic_factory;

	ID2D1Bitmap* current_image_direct2d = nullptr;
	IWICBitmapSource* current_image_wic = nullptr;

	bool init(const View_Window_Init_Params& params);
	bool discard();
	
	bool set_current_image(IWICBitmapSource* image);
	bool set_file_name(const wchar_t* file_name, size_t length);

	bool get_client_area(int* width, int* height);

	int enter_message_loop();
	LRESULT __stdcall wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};