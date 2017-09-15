#pragma once
#include <Windows.h>

#include "string.hpp"
#include "sequence.hpp"


struct File_Info
{
    String path;
    DWORD file_attributes = 0;
    FILETIME creation_time{ 0 };
    FILETIME last_access_time{ 0 };
    FILETIME last_write_time{ 0 };
    DWORD file_size = 0;
};

typedef bool(*Get_Folder_Files_Filter)(const WIN32_FIND_DATA& file_data, void* userdata);

struct File_System_Utility
{
    static HRESULT get_folder_files(
        Sequence<File_Info>* output,
        const String& folder_path, 
        Get_Folder_Files_Filter filter, 
        void* userdata = nullptr,
        IAllocator* file_allocator = g_standard_allocator);

    static bool folder_exists(const String& folder_path);
    static HRESULT extract_folder_path(const String& file_path, String* folder_path, IAllocator* folder_path_allocator = g_standard_allocator);
    static bool extract_file_name_from_path(const String& file_path, String* file_name, IAllocator* allocator = g_standard_allocator);

    // Windows Explorer API
    static HRESULT select_file_in_explorer(const String& file_path);
    static HRESULT normalize_path(String& file_path);
};
