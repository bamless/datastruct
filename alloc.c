#include "alloc.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: make this configurable
// TODO: use _AlignOf for type allocations
#define EXT_DEFAULT_ALIGN 16

// -----------------------------------------------------------------------------
// Allocator context & default allocator
//

typedef struct {
    Allocator base;
} DefaultAllocator;

static void *default_alloc(Allocator *a, size_t size) {
    (void)a;
    void *mem = malloc(size);
    if(!mem) {
        fprintf(stderr, "%s:%d: memory allocation failed: %s\n", __FILE__, __LINE__,
                strerror(errno));
        abort();
    }
    return mem;
}

static void *default_realloc(Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    (void)a;
    (void)old_size;
    void *mem = realloc(ptr, new_size);
    if(!mem) {
        fprintf(stderr, "%s:%d: memory allocation failed: %s\n", __FILE__, __LINE__,
                strerror(errno));
        abort();
    }
    return mem;
}

static void default_free(Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)size;
    free(ptr);
}

static DefaultAllocator default_allocator = {
    {
        .alloc = default_alloc,
        .realloc = default_realloc,
        .free = default_free,
        .prev = NULL,
    },
};

Ext_Allocator *ext_allocator_ctx = (Ext_Allocator *)&default_allocator;

void ext_push_allocator(Ext_Allocator *alloc) {
    assert(!alloc->prev && "Allocator already on stack. Pop it first");
    alloc->prev = ext_allocator_ctx;
    ext_allocator_ctx = alloc;
}

Allocator *ext_pop_allocator(void) {
    assert(ext_allocator_ctx->prev && "Mismatched allocator pushes and pops");
    Allocator *cur = ext_allocator_ctx;
    ext_allocator_ctx = ext_allocator_ctx->prev;
    cur->prev = NULL;
    return cur;
}

// -----------------------------------------------------------------------------
// Global temp allocator
//

static void *temp_allocate_wrap(Ext_Allocator *a, size_t size) {
    Ext_TempAllocator *tmp = (Ext_TempAllocator *)a;
    ptrdiff_t available = tmp->end - tmp->start;
    ptrdiff_t alignment = -size & (EXT_DEFAULT_ALIGN - 1);
    if((ptrdiff_t)size > available - alignment) {
        fprintf(stderr, "%s:%d: temp allocation failed: %zu bytes requested, %zu bytes available\n",
                __FILE__, __LINE__, size, available);
        abort();
    }
    return tmp->end -= size + alignment;
}

static void *temp_reallocate_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    Ext_TempAllocator *tmp = (Ext_TempAllocator *)a;
    // Reallocating last allocated memory, can grow in-place
    if(ptr == tmp->end) {
        ptrdiff_t alignment = -old_size & (EXT_DEFAULT_ALIGN - 1);
        tmp->end += old_size + alignment;
        return temp_allocate_wrap(a, new_size);
    } else {
        void *new_ptr = temp_allocate_wrap(a, new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    }
}

static void temp_deallocate_wrap(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)ptr;
    (void)size;
}

static char temp_mem[EXT_ALLOC_TEMP_SIZE];
Ext_TempAllocator ext_temp_allocator = {
    {
        .alloc = temp_allocate_wrap,
        .realloc = temp_reallocate_wrap,
        .free = temp_deallocate_wrap,
        .prev = NULL,
    },
    .start = temp_mem,
    .end = temp_mem + EXT_ALLOC_TEMP_SIZE,
};

void *ext_temp_allocate(size_t size) {
    return ext_temp_allocator.base.alloc((Allocator *)&ext_temp_allocator, size);
}

void *ext_temp_reallocate(void *ptr, size_t old_size, size_t new_size) {
    return ext_temp_allocator.base.realloc((Allocator *)&ext_temp_allocator, ptr, old_size,
                                           new_size);
}

void ext_temp_reset(void) {
    ext_temp_allocator.start = temp_mem;
    ext_temp_allocator.end = temp_mem + EXT_ALLOC_TEMP_SIZE;
}

void ext_push_temp_allocator(void) {
    ext_push_allocator((Ext_Allocator *)&ext_temp_allocator);
}
