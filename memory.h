#pragma once

#include "ctk/ctk.h"
#include "ctk/memory.h"

struct MemBase {
    CTK_Stack *perma;
    CTK_Stack *temp;
};

static MemBase *create_mem_base(u32 total_size, u32 temp_size) {
    CTK_ASSERT(total_size > temp_size);

    CTK_Stack *perma_stack = ctk_create_stack(total_size);
    auto mem = ctk_alloc<MemBase>(perma_stack, 1);
    mem.perma = perma_stack;
    mem.temp = ctk_create_stack(CTK_MEGABYTE, &perma_stack->allocator);
    return mem;
}
