#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#include "alloc.h"

typedef enum {
    EXT_ARENA_NONE = 0,
    // Forces the arena to behave like a stack allocator. The arena will
    // check that frees are done only in LIFO order. If this is not the
    // case, it will abort with an error.
    EXT_ARENA_STACK_ALLOC = 1 << 0,
    // Zeroes the memory allocated by the arena.
    EXT_ARENA_ZERO_ALLOC = 1 << 1,
} Ext_ArenaFlags;

typedef struct Ext_ArenaPage {
    struct Ext_ArenaPage *next;
    char *start, *end;
    char data[];
} Ext_ArenaPage;

// TODO: implement maximum size
typedef struct Ext_Arena {
    Ext_Allocator base;
    const size_t alignment;
    const size_t page_size;
    Ext_Allocator *const page_allocator;
    const Ext_ArenaFlags flags;
    Ext_ArenaPage *first_page, *last_page;
    size_t allocated;
} Ext_Arena;

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags);

void *ext_arena_alloc(Ext_Arena *a, size_t size);
void *ext_arena_realloc(Ext_Arena *a, void *ptr, size_t old_size, size_t new_size);
void ext_arena_dealloc(Ext_Arena *a, void *ptr, size_t size);
void ext_arena_reset(Ext_Arena *a);
void ext_arena_free(Ext_Arena *a);
char *ext_arena_strdup(Ext_Arena *a, const char *str);
char *ext_arena_sprintf(Ext_Arena *a, const char *fmt, ...);
char *ext_arena_vsprintf(Ext_Arena *a, const char *fmt, va_list ap);

#endif
