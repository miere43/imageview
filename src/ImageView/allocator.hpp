#pragma once

struct IAllocator
{
    virtual void* allocate(size_t size) = 0;
    virtual void* reallocate(void* block, size_t new_size) = 0;
    virtual void  deallocate(void* block) = 0;
};

struct Standard_Allocator : public IAllocator
{
    virtual void* allocate(size_t size) override final;
    virtual void* reallocate(void* block, size_t new_size) override final;
    virtual void  deallocate(void* block) override final;
};

struct Temporary_Allocator : public IAllocator
{
    void* block = nullptr;
    void* current = nullptr;
    void* max = nullptr;
    // Allocator used to allocate 'block'.
    IAllocator* block_allocator = nullptr;

    size_t alignment = sizeof(void*);

    Temporary_Allocator(IAllocator* block_allocator);

    // Changes size of 'block'. Returns false on failure, no state is changed in that case.
    bool set_size(size_t new_size);
    bool clear();

    virtual void* allocate(size_t size) override final;
    virtual void* reallocate(void * block, size_t new_size) override final;
    virtual void  deallocate(void* block) override final;
};

extern Standard_Allocator* g_standard_allocator;
extern Temporary_Allocator* g_temporary_allocator;

// Remembers 'current' pointer of temporary allocator and reset's it back when out of scope.
struct Temporary_Allocator_Guard
{
    Temporary_Allocator* allocator = nullptr;
    void* prev_current_state = nullptr;

    Temporary_Allocator_Guard(Temporary_Allocator* allocator = g_temporary_allocator);
    ~Temporary_Allocator_Guard();
};
