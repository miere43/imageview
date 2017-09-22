#pragma once
#include "allocator.hpp"


struct Pool_Allocator : public IAllocator
{
    struct Block
    {
        Block* next;
        int used_slots;
    };

    IAllocator* block_allocator = nullptr;

    Pool_Allocator(int block_size, IAllocator* block_allocator = g_standard_allocator);
    ~Pool_Allocator();

    virtual void* allocate(size_t size) override final;
    virtual void* reallocate(void* block, size_t new_size) override final;
    virtual void  deallocate(void* block) override final;
private:
    int block_size = 0;
    int blocks_allocated = 0;
    int total_slots_used = 0;
    Block* first = nullptr;
    Block* last  = nullptr;

    Block* push_new_block();
    void* occupy_slot(Block* block, int slot_index);
    void  free_slot(Block* block, void* slot_value);
    void  free_all_slots(Block* block);
    int   get_free_slot_index(Block* block);
};
