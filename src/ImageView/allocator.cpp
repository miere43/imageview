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

void* Standard_Allocator::reallocate(void* block, size_t new_size)
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
        block = current = block_max = nullptr;
    }

    block = current = new_block;
    block_max = (unsigned char*)block + new_size;

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

    if (new_block_end > (size_t)block_max)
        return nullptr; // @TODO: allocate using standard allocator.

    current = (void*)(new_block_end);
    return (void*)new_block;
}

void* Temporary_Allocator::reallocate(void* block, size_t new_size)
{
    if (block == nullptr)
        return allocate(new_size);
    
    E_VERIFY_R(block >= this->block && block < this->block_max, nullptr);

    if ((size_t)block + (size_t)new_size > (size_t)this->block_max)
        return nullptr;

    return block;
}

void Temporary_Allocator::deallocate(void* block)
{
    // I don't care.
}

Temporary_Allocator_Guard::Temporary_Allocator_Guard(Temporary_Allocator* allocator)
    : allocator(allocator)
{
    E_VERIFY_NULL(allocator);

    prev_current_state = allocator->current;
}

Temporary_Allocator_Guard::~Temporary_Allocator_Guard()
{
    if (allocator != nullptr && prev_current_state != nullptr)
    {
        allocator->current = prev_current_state;
        prev_current_state = nullptr;
    }
}