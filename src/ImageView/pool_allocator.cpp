#include "pool_allocator.hpp"
#include "error.hpp"

static const int ITEMS_PER_BLOCK = 32;
static const size_t block_items_offset = sizeof(Pool_Allocator::Block*) + sizeof(int);


Pool_Allocator::Pool_Allocator(int block_size, IAllocator* block_allocator)
{
    E_VERIFY_NULL(block_allocator);
    E_VERIFY(block_size > 0);

    this->block_size = block_size;
    this->block_allocator = block_allocator;

    first = push_new_block();
    last  = first;
}

Pool_Allocator::~Pool_Allocator()
{
    Block* block = first;
    while (block != nullptr)
    {
        Block* next = block->next;
        free_all_slots(block);
        block = next;
    }
}

void* Pool_Allocator::allocate(size_t size)
{
    E_VERIFY_R(size == block_size, nullptr);
    Block* block;

    if (total_slots_used == blocks_allocated * ITEMS_PER_BLOCK)
        block = push_new_block();
    else
        block = first;

    if (block == nullptr)
        return nullptr;

get_slot:
    int free_slot_index = get_free_slot_index(block);
    if (free_slot_index == -1)
    {
        if (block->next != nullptr)
        {
            block = block->next;
            goto get_slot;
        }
        else
        {
            block = push_new_block();
            free_slot_index = 0;
        }
    }

    ++total_slots_used;
    return occupy_slot(block, free_slot_index);
}

void* Pool_Allocator::reallocate(void* block, size_t new_size)
{
    E_VERIFY_R(new_size == block_size, nullptr);
    if (block == nullptr)
        return allocate(new_size);

    return block;
}

void Pool_Allocator::deallocate(void* value)
{
    if (value == nullptr)
        return;

    Block* block = first;
    do
    {
        if ((void*)value >= (void*)block
            && (void*)value <= (void*)((size_t)block + (block_size * ITEMS_PER_BLOCK)))
        {
            free_slot(block, value);
            return;
        }
    } while (block = block->next);

    E_VERIFY(false); // not allocated by this allocator
}

Pool_Allocator::Block* Pool_Allocator::push_new_block()
{
    E_VERIFY_NULL_R(block_allocator, nullptr);

    Block* new_block = (Block*)block_allocator->allocate(block_items_offset + (block_size * ITEMS_PER_BLOCK));
    if (new_block == nullptr)
        return nullptr;

    ++blocks_allocated;

    new_block->used_slots = 0;
    new_block->next = nullptr;

    if (last != nullptr)
    {
        last->next = new_block;
        last = new_block;
    }

    return new_block;
}

void * Pool_Allocator::occupy_slot(Block* block, int slot_index)
{
    E_VERIFY_NULL_R(block, nullptr);
    E_VERIFY_R(slot_index >= 0 && slot_index < ITEMS_PER_BLOCK, nullptr);
    E_VERIFY_R((block->used_slots & (1 << slot_index)) == false, nullptr);

    block->used_slots = block->used_slots | (1 << slot_index);
    return (char*)block + block_items_offset + (block_size * slot_index);
}

void Pool_Allocator::free_slot(Block* block, void* slot_value)
{
    E_VERIFY_NULL(block);
    E_VERIFY_NULL(slot_value);
    
    size_t slot_index = (((size_t)slot_value - ((size_t)block + block_items_offset)) / block_size);

    E_VERIFY(slot_index >= 0 && slot_index < ITEMS_PER_BLOCK);

    block->used_slots = block->used_slots & (~(1 << slot_index));
}

void Pool_Allocator::free_all_slots(Block* block)
{
    E_VERIFY_NULL(block);

    block->used_slots = 0;
}

int Pool_Allocator::get_free_slot_index(Block* block)
{
    E_VERIFY_NULL_R(block, -1);

    if (block->used_slots == 0xFFFFFFFF)
        return -1;

    for (int i = 0; i < ITEMS_PER_BLOCK; ++i)
    {
        int is_occupied = block->used_slots & (1 << i);
        if (!is_occupied)
            return i;
    }

    return -1;
}
