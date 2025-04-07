#include "arena.h"

#include <assert.h>
#include <stdbool.h>

#include "alloc.h"

static void *arena_alloc(Ext_Allocator *a, size_t size) {
    (void)a;
    (void)size;
    assert(false && "TODO");
}

void *arena_realloc(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    (void)a;
    (void)ptr;
    (void)old_size;
    (void)new_size;
    assert(false && "TODO");
}

void arena_dealloc(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)ptr;
    (void)size;
    assert(false && "TODO");
}

Ext_Arena new_arena(Ext_Allocator *a, size_t size, size_t alignment, Ext_Arena_Opts flags) {
    char *mem = a->alloc(a, size);
    Ext_Arena arena = {
        .base = {.alloc = &arena_alloc, .realloc = &arena_realloc, .free = &arena_dealloc},
        .size = size,
        .alignment = alignment,
        .flags = flags,
        .mem = mem,
        .start = mem,
        .end = mem + size,
    };
    return arena;
}
