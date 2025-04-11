#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#include "alloc.h"

typedef enum {
    /* Forces the arena to behave like a bump allocator. The arena will
       check that frees are done only in LIFO order. If this is not the
       case, it will abort with an error. */
    EXT_ARENA_BUMP_ALLOC = 1 << 0,
    /* Zeroes the memory allocated by the arena. */
    EXT_ARENA_ZERO_ALLOC = 1 << 1,
} Ext_ArenaFlags;

typedef struct Ext_ArenaPage {
    struct Ext_ArenaPage* next;
    char *start, *end;
    char data[];
} Ext_ArenaPage;

typedef struct Ext_Arena {
    Ext_Allocator base;
    const size_t alignment;
    const size_t page_size;
    Allocator* const page_allocator;
    const Ext_ArenaFlags flags;
    Ext_ArenaPage *first_page, *last_page;
    size_t allocated;
} Ext_Arena;

Ext_Arena new_arena(Ext_Allocator* page_alloc, size_t alignment, size_t page_size,
                    Ext_ArenaFlags flags);
void arena_destroy(Ext_Arena* a);

#endif
