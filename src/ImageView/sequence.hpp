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
    if (!reserve(count + 1))
        return false;

    data[count++] = value;
    return true;
}

template<typename T>
inline bool Sequence<T>::reserve(int reserve_capacity)
{
    E_VERIFY_R(reserve_capacity >= 0, false);

    if (reserve_capacity <= capacity)
        return true;

    size_t new_data_size = sizeof(T) * reserve_capacity;

    T* new_data = (T*)allocator->allocate(new_data_size);
    if (new_data == nullptr)
        return false;

    if (data != nullptr && count > 0)
        memcpy_s(new_data, new_data_size, data, sizeof(T) * count);

    data = new_data;
    capacity = reserve_capacity;

    return true;
}
