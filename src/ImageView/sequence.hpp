#pragma once
#include <memory.h>

#include "allocator.hpp"
#include "error.hpp"


template<typename T>
struct Sequence
{
    T* data = nullptr;
    int count = 0;
    int capacity = 0;
    IAllocator* allocator = nullptr;

    Sequence(int count = 0, IAllocator* allocator = g_standard_allocator);

    bool push_back(const T& value);
    bool reserve(int reserve_capacity);

    bool is_empty() const;
    bool is_valid_index(int index) const;
private:
    int  find_capacity(int cap);
    bool ensure_capacity(int required_capacity);
};

template<typename T>
inline Sequence<T>::Sequence(int count, IAllocator * allocator)
{
    E_VERIFY(count >= 0);
    E_VERIFY_NULL(allocator);

    this->count = count;
    this->allocator = allocator;
}

template<typename T>
inline bool Sequence<T>::push_back(const T& value)
{
    if (!ensure_capacity(count + 1))
        return false;

    data[count++] = value;
    return true;
}

template<typename T>
inline bool Sequence<T>::reserve(int required_capacity)
{
    E_VERIFY_R(required_capacity >= 0, false);

    if (capacity >= required_capacity && data != nullptr)
        return true;

    size_t new_data_size = sizeof(T) * required_capacity;

    T* new_data = (T*)allocator->reallocate(data, new_data_size);
    if (new_data == nullptr)
        return false;

    //if (data != nullptr && count > 0)
    //    memcpy_s(new_data, new_data_size, data, sizeof(T) * count);

    data = new_data;
    capacity = required_capacity;

    return true;
}

template<typename T>
inline bool Sequence<T>::is_empty() const
{
    return data == nullptr || count <= 0;
}

template<typename T>
inline bool Sequence<T>::is_valid_index(int index) const
{
    if (is_empty())
        return false;

    return index >= 0 && index < count;
}

template<typename T>
inline int Sequence<T>::find_capacity(int cap)
{
    return cap <= 5 ? 10 : this->capacity * 2;
}

template<typename T>
inline bool Sequence<T>::ensure_capacity(int required_capacity)
{
    E_VERIFY_R(required_capacity > 0, false);
    if (count < capacity && data != nullptr)
        return true;

    int new_cap = find_capacity(required_capacity);
    return reserve(new_cap);
}
