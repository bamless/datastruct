#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "context.h"

#define ARENA_DEFAULT_PAGE_SIZE (4 * 1024)  // 4 KiB
#define ARENA_DEFAULT_ALIGNMENT (16)

#define ARENA_ALIGN(o, a) (-(uintptr_t)(o) & ((a)->alignment - 1))

static Ext_ArenaPage *new_page(Ext_Arena *arena) {
    Ext_ArenaPage *page = arena->page_allocator->allocate(arena->page_allocator, arena->page_size);
    page->next = NULL;
    // Account for alignment of first allocation; the arena assumes every pointer starts aligned to
    // the arena's alignment.
    page->start = page->data + ARENA_ALIGN(page->data, arena);
    page->end = page->data + arena->page_size;
    return page;
}

static void *alloc_wrap(Ext_Allocator *a, size_t size) {
    return ext_arena_alloc((Ext_Arena *)a, size);
}

static void *realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    return ext_arena_realloc((Ext_Arena *)a, ptr, old_size, new_size);
}

static void dealloc_wrap(Ext_Allocator *a, void *ptr, size_t size) {
    ext_arena_dealloc((Ext_Arena *)a, ptr, size);
}

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags) {
    if(!alignment) alignment = ARENA_DEFAULT_ALIGNMENT;
    if(!page_size) page_size = ARENA_DEFAULT_PAGE_SIZE;
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");
    assert(page_size > sizeof(Ext_ArenaPage) + alignment &&
           "Page size must be greater the size of Ext_ArenaPage + alignment bytes");
    return (Ext_Arena){
        .base =
            {
                .allocate = alloc_wrap,
                .reallocate = realloc_wrap,
                .deallocate = dealloc_wrap,
            },
        .alignment = alignment,
        .page_size = page_size,
        .page_allocator = page_alloc ? page_alloc : ext_context->alloc,
        .flags = flags,
    };
}

void *ext_arena_alloc(Ext_Arena *a, size_t size) {
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

void *ext_arena_realloc(Ext_Arena *a, void *ptr, size_t old_size, size_t new_size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    assert(ARENA_ALIGN(old_size, arena) == 0 && "Old size is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    assert(page && "No pages in arena");

    size_t alignment = ARENA_ALIGN(ptr, arena);
    if(page->start - old_size - alignment == ptr) {
        // Reallocating last allocated memory, can grow/shrink in-place
        page->start -= old_size + alignment;
        arena->allocated -= old_size + alignment;
        return ext_arena_alloc(a, new_size);
    } else if(new_size > old_size) {
        void *new_ptr = ext_arena_alloc(a, new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    } else {
        return ptr;
    }
}

void ext_arena_dealloc(Ext_Arena *a, void *ptr, size_t size) {
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

void ext_arena_reset(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        page->start = page->data + ARENA_ALIGN(page->data, a);
        page->end = page->data + a->page_size;
        page = page->next;
    }
    a->last_page = a->first_page;
    a->allocated = 0;
}

void ext_arena_free(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        Ext_ArenaPage *next = page->next;
        a->page_allocator->deallocate(a->page_allocator, page, a->page_size);
        page = next;
    }
    a->first_page = NULL;
    a->last_page = NULL;
    a->allocated = 0;
}

char *ext_arena_strdup(Ext_Arena *a, const char *str) {
    size_t n = strlen(str);
    char *res = ext_arena_alloc(a, n + 1);
    memcpy(res, str, n);
    res[n] = '\0';
    return res;
}

char *ext_arena_sprintf(Ext_Arena *a, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *res = ext_arena_vsprintf(a, fmt, ap);
    va_end(ap);
    return res;
}

char *ext_arena_vsprintf(Ext_Arena *a, const char *fmt, va_list ap) {
    va_list cpy;
    va_copy(cpy, ap);
    int n = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);

    assert(n >= 0 && "error in vsnprintf");
    char *res = ext_arena_alloc(a, n + 1);

    va_copy(cpy, ap);
    vsnprintf(res, n + 1, fmt, cpy);
    va_end(cpy);

    return res;
}
