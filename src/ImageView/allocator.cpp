#include "allocator.hpp"
#include <stdlib.h>
#include "error.hpp"


Standard_Allocator  g_standard_allocator_obj;
Standard_Allocator* g_standard_allocator = &g_standard_allocator_obj;

Temporary_Allocator  g_temporary_allocator_obj{ g_standard_allocator };
Temporary_Allocator* g_temporary_allocator = &g_temporary_allocator_obj;


void* Standard_Allocator::allocate(size_t size)
{
    return malloc(size);
}

void Standard_Allocator::deallocate(void* block)
{
    free(block);
}

void * Standard_Allocator::reallocate(void * block, size_t new_size)
{
    return realloc(block, new_size);
}

Temporary_Allocator::Temporary_Allocator(IAllocator* block_allocator)
{
    E_VERIFY_NULL(block_allocator);
    this->block_allocator = block_allocator;
}

bool Temporary_Allocator::set_size(size_t new_size)
{
    E_VERIFY_R(current == block, false); // Make sure no stuff is allocated.

    void* new_block = block_allocator->allocate(new_size);
    if (new_block == nullptr)
        return false;

    if (block != nullptr)
    {
        block_allocator->deallocate(block);
        block = current = max = nullptr;
    }

    block = current = new_block;
    max = (unsigned char*)block + new_size;

    return true;
}

bool Temporary_Allocator::clear()
{
    current = block;
    return true;
}

void* Temporary_Allocator::allocate(size_t size)
{
    // Align pointer to the next block.
    size_t new_block = (size_t)current + (size_t)current % alignment;
    size_t new_block_end = new_block + size;

    if (new_block_end > (size_t)max)
        return nullptr; // @TODO: allocate using standard allocator.

    current = (void*)(new_block_end);
    return (void*)new_block;
}

void* Temporary_Allocator::reallocate(void* block, size_t new_size)
{
    return block;
}

void Temporary_Allocator::deallocate(void * block)
{
    // I don't care.
}
