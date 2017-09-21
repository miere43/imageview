#pragma once
#include "allocator.hpp"
#include "string.hpp"
#include "string_builder.hpp"


// @TODO: support UTF-8
struct Line_Reader
{
    const char* source = nullptr;
    int source_count = 0;
    String_Builder* line;
    int index = -1;

    void set_string_builder(String_Builder* sb);
    void set_source(const char* source, int source_count);
    
    bool next_line();
private:
    bool set_line(int i, int skip);
};