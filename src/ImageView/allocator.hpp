#pragma once

struct IAllocator
{
    virtual void* allocate(size_t size) = 0;
    virtual void* reallocate(void* block, size_t new_size) = 0;
    virtual void  deallocate(void* block) = 0;
};

struct Standard_Allocator : public IAllocator
{
    void* allocate(size_t size) override;
    void* reallocate(void* block, size_t new_size) override;
    void  deallocate(void* block) override;
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

    void* allocate(size_t size) override;
    void* reallocate(void * block, size_t new_size) override;
    void  deallocate(void* block) override;
};

extern Standard_Allocator* g_standard_allocator;
extern Temporary_Allocator* g_temporary_allocator;

