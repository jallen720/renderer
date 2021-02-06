#pragma once

#include "ctk/ctk.h"
#include "ctk/memory.h"

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
struct Memory {
    CTK_Stack perma;
    CTK_Stack temp;
    CTK_FreeList free_list;
};

struct Core {
    Memory mem;
};

#include "renderer/core_inputs.h"

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static Core *create_core() {
    // Memory
    CTK_Stack perma_stack = ctk_create_stack(4 * CTK_KILOBYTE);
    auto core = ctk_alloc<Core>(&perma_stack, 1);
    core->mem.perma = perma_stack;
    core->mem.temp = ctk_create_stack(&core->mem.perma, CTK_KILOBYTE);
    core->mem.free_list = ctk_create_free_list(4 * CTK_MEGABYTE);
    return core;
}
