#include <wchar.h>

#include "file_system_utility.hpp"
#include "error.hpp"
#include "string.hpp"


HRESULT File_System_Utility::get_folder_files(
    Sequence<File_Info>* output,
    const String& folder_path,
    Get_Folder_Files_Filter filter, 
    void* userdata,
    IAllocator* file_allocator)
{
    E_VERIFY_R(output, E_INVALIDARG);
    E_VERIFY_NULL_R(filter, E_INVALIDARG);
    E_VERIFY_NULL_R(file_allocator, E_INVALIDARG);

    if (!folder_exists(folder_path))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    bool ends_with_slash = folder_path.ends_with('/');
    if (!ends_with_slash)
        ends_with_slash = folder_path.ends_with('\\');

    size_t path_size = folder_path.calc_size() + (ends_with_slash * sizeof(wchar_t)) + (sizeof(wchar_t) * 2);
    
    void* prev_temp_current = g_temporary_allocator->current;
    wchar_t* temp_folder_path = (wchar_t*)g_temporary_allocator->allocate(path_size);

    if (temp_folder_path == nullptr)
        return E_OUTOFMEMORY;

    memcpy_s(temp_folder_path, path_size, folder_path.data, folder_path.calc_size_no_zero_terminator());
    int char_index = folder_path.count;
    if (!ends_with_slash)
        temp_folder_path[char_index++] = L'\\';
    temp_folder_path[char_index++] = L'*';
    temp_folder_path[char_index] = L'\0';

    WIN32_FIND_DATA file;
    HANDLE search_handle = FindFirstFileExW(temp_folder_path, FindExInfoBasic, &file, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    g_temporary_allocator->current = prev_temp_current;

    if (search_handle == INVALID_HANDLE_VALUE)
        return E_HANDLE;

    Sequence<File_Info> files;
    while (FindNextFileW(search_handle, &file))
    {
        if (filter(file, userdata))
        {
            File_Info info;
            info.path = String::duplicate(file.cFileName, wcslen(file.cFileName));
            info.file_attributes = file.dwFileAttributes;
            info.creation_time = file.ftCreationTime;
            info.last_access_time = file.ftLastAccessTime;
            info.last_write_time = file.ftLastWriteTime;
            info.file_size = file.nFileSizeLow;

            files.push_back(info);
        }
    }

    FindClose(search_handle);
    *output = files;

    return S_OK;
}

bool File_System_Utility::folder_exists(const String& folder_path)
{
    DWORD a = GetFileAttributesW(folder_path.data);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

HRESULT File_System_Utility::extract_folder_path(const String& file_path, String* folder_path, IAllocator* folder_path_allocator)
{
    E_VERIFY_NULL_R(folder_path, E_INVALIDARG);
    E_VERIFY_NULL_R(folder_path_allocator, E_INVALIDARG);
    
    if (String::is_null_or_empty(file_path))
        return false;

    void* temp_current = g_temporary_allocator->current;
    int slash_index = file_path.last_index_of('/');
    if (slash_index == -1)
        slash_index = file_path.last_index_of('\\');

    if (slash_index == -1)
        slash_index = file_path.count - 1;

    String result = String::allocate_string_of_length(slash_index, folder_path_allocator);
    if (String::is_null(result))
        return E_OUTOFMEMORY;

    wmemcpy(result.data, file_path.data, slash_index);
    result.data[result.count] = L'\0';

    *folder_path = result;
    return S_OK;
}

bool File_System_Utility::extract_file_name_from_path(const String& file_path, String* file_name, IAllocator* allocator)
{
    E_VERIFY_NULL_R(file_name, false);
    E_VERIFY_NULL_R(allocator, false);
    if (String::is_null_or_empty(file_path))
        return false;

    int slash_index = file_path.last_index_of(L'/');
    if (slash_index == -1)
        slash_index = file_path.last_index_of(L'\\');

    String result = file_path.substring(slash_index + 1, -1, allocator);
    if (String::is_null_or_empty(result))
        return false;

    *file_name = result;
    return true;
}
