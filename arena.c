#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"

#define ARENA_DEFAULT_PAGE_SIZE (4 * 1024)  // 4 KiB
#define ARENA_DEFAULT_ALIGNMENT (16)

#define ARENA_ALIGN(o, a) (-(uintptr_t)(o) & ((a)->alignment - 1))

static Ext_ArenaPage *new_page(Ext_Arena *arena) {
    Ext_ArenaPage *page = arena->page_allocator->alloc(arena->page_allocator, arena->page_size);
    page->next = NULL;
    page->start = page->data;
    page->end = page->start + arena->page_size;

    // Account for alignment of first allocation; the arena assumes every pointer starts
    // aligned to the arena's alignment.
    size_t data_align = -(uintptr_t)page->data & (arena->alignment - 1);
    page->start += data_align;

    return page;
}

static void *arena_alloc(Ext_Allocator *a, size_t size) {
    Ext_Arena *arena = (Ext_Arena *)a;

    size_t header_alignment = ARENA_ALIGN(sizeof(Ext_ArenaPage), arena);
    if(size > arena->page_size - (sizeof(Ext_ArenaPage) + header_alignment)) {
        fprintf(stderr, "Error: requested size %zu exceeds max allocatable size in page (%zu)\n",
                size, arena->page_size - (sizeof(Ext_ArenaPage) + header_alignment));
        abort();
    }

    if(!arena->last_page) {
        assert(arena->first_page == NULL && arena->allocated == 0);
        Ext_ArenaPage *page = new_page(arena);
        arena->first_page = page;
        arena->last_page = page;
    }

    size_t alignment = ARENA_ALIGN(size, arena);
    ptrdiff_t available = arena->last_page->end - arena->last_page->start - alignment;
    while(available < (ptrdiff_t)size) {
        Ext_ArenaPage *next_page = arena->last_page->next;
        if(!next_page) {
            arena->last_page->next = new_page(arena);
            arena->last_page = arena->last_page->next;
            available = arena->last_page->end - arena->last_page->start - alignment;
            break;
        } else {
            arena->last_page = next_page;
            available = next_page->end - next_page->start - alignment;
        }
    }

    assert(available >= (ptrdiff_t)size && "Not enough space in arena");

    void *p = arena->last_page->start;
    assert(ARENA_ALIGN(p, arena) == 0 && "Pointer is not aligned to the arena's alignment");
    arena->last_page->start += size + alignment;
    arena->allocated += size + alignment;

    if(arena->flags & EXT_ARENA_ZERO_ALLOC) {
        memset(p, 0, size);
    }

    return p;
}

void *arena_realloc(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    assert(ARENA_ALIGN(old_size, arena) == 0 && "Old size is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    assert(page && "No pages in arena");

    size_t alignment = ARENA_ALIGN(ptr, arena);
    if(page->start - old_size - alignment == ptr) {
        // Reallocating last allocated memory, can grow in-place
        page->start -= old_size + alignment;
        arena->allocated -= old_size + alignment;
        return arena_alloc(a, new_size);
    } else {
        void *new_ptr = arena_alloc(a, new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    }
}

void arena_dealloc(Ext_Allocator *a, void *ptr, size_t size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    assert(ARENA_ALIGN(size, arena) == 0 && "Size is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    assert(page && "No pages in arena");

    size_t alignment = ARENA_ALIGN(size, arena);
    if(page->start - size - alignment == ptr) {
        // Deallocating last allocated memory, can shrink in-place
        page->start -= size + alignment;
        arena->allocated -= size + alignment;
        return;
    }

    // In stack allocator mode force LIFO order
    if(arena->flags & EXT_ARENA_STACK_ALLOC) {
        fprintf(stderr, "Deallocating memory in non-LIFO order: got %p, expected %p\n", ptr,
                (void *)(page->start - size - alignment));
        abort();
    }
}

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags) {
    if(!alignment) alignment = ARENA_DEFAULT_ALIGNMENT;
    if(!page_size) page_size = ARENA_DEFAULT_PAGE_SIZE;
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");
    assert(page_size > sizeof(Ext_ArenaPage) + alignment &&
           "Page size must be greater the size of Ext_ArenaPage + alignment bytes");
    return (Ext_Arena){
        .base = {.alloc = arena_alloc, .realloc = arena_realloc, .free = arena_dealloc},
        .alignment = alignment,
        .page_size = page_size,
        .page_allocator = page_alloc ? page_alloc : ext_allocator_ctx,
        .flags = flags,
    };
}

void ext_arena_reset(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    size_t header_alignment = ARENA_ALIGN(sizeof(Ext_ArenaPage), a);
    while(page) {
        page->start = page->data + header_alignment;
        page->end = page->start + a->page_size;
        page = page->next;
    }
    a->last_page = a->first_page;
    a->allocated = 0;
}

void ext_arena_destroy(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        Ext_ArenaPage *next = page->next;
        a->page_allocator->free(a->page_allocator, page, a->page_size);
        page = next;
    }
    a->first_page = NULL;
    a->last_page = NULL;
    a->allocated = 0;
}
