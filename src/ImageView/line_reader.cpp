#include "line_reader.hpp"
#include "error.hpp"


void Line_Reader::set_string_builder(String_Builder* sb)
{
    this->line = sb;
}

void Line_Reader::set_source(const char* source, int source_count)
{
    this->source = source;
    this->source_count = source_count;
    this->index  = 0;
}

bool Line_Reader::next_line()
{
    E_VERIFY_NULL_R(source, false);
    E_VERIFY_R(source_count >= 0, false);
    E_VERIFY_NULL_R(line, false);
    E_VERIFY_R(index >= 0, false);

    if (index >= source_count)
        return false;

    for (int i = index; i < source_count; ++i)
    {
        char c = source[i];
        if (c == '\r' && i + 1 < source_count && source[i + 1] == '\n')
            return set_line(i, 2);
        else if (c == '\n')
            return set_line(i, 1);
        else
            continue;
    }

    return set_line(source_count, 0);
}

bool Line_Reader::set_line(int i, int skip)
{
    int base_index = index;
    int line_count = i - base_index;
    index = i + skip;

    if (!line->reserve(line_count + 1))
        return false;

    for (int j = 0; j < line_count; ++j)
        line->buffer[j] = static_cast<wchar_t>(source[base_index + j]);
    line->buffer[line_count] = L'\0';
    
    return true;
}
