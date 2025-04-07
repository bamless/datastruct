#ifndef ARENA_H
#define ARENA_H

#include "alloc.h"

typedef enum {
    EXT_ARENA_BUMP_ALLOC = 1 << 0,
    EXT_ARENA_ZERO_ALLOC = 1 << 1,
    EXT_ARENA_FIXED = 1 << 2,
} Ext_Arena_Opts;

typedef struct Ext_Arena {
    Ext_Allocator base;
    size_t size, alignment;
    Ext_Arena_Opts flags;
    char* mem;
    char *start, *end;
} Ext_Arena;

Ext_Arena new_arena(Ext_Allocator* alloc, size_t size, size_t alignment, Ext_Arena_Opts flags);

#endif
