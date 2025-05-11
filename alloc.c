#include "alloc.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Allocator context & default allocator
//

static void *default_alloc(Ext_Allocator *a, size_t size) {
    (void)a;
    void *mem = malloc(size);
    if(!mem) {
        fprintf(stderr, "%s:%d: memory allocation failed: %s\n", __FILE__, __LINE__,
                strerror(errno));
        abort();
    }
    return mem;
}

static void *default_realloc(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
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

static void default_free(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)size;
    free(ptr);
}

// -----------------------------------------------------------------------------
// Default allocator
//

Ext_DefaultAllocator ext_default_allocator = {
    {.alloc = default_alloc, .realloc = default_realloc, .free = default_free},
};

// -----------------------------------------------------------------------------
// Global temp allocator
//

#define EXT_DEFAULT_ALIGN 16
#ifndef EXT_DEAFULT_TEMP_SIZE
    #define EXT_DEAFULT_TEMP_SIZE (256 * 1024 * 1024)
#endif

static void *temp_alloc_wrap(Ext_Allocator *a, size_t size);
static void *temp_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size);
static void temp_free_wrap(Ext_Allocator *a, void *ptr, size_t size);

static char temp_mem[EXT_DEAFULT_TEMP_SIZE];
EXT_TLS Ext_TempAllocator ext_temp_allocator = {
    {
        .alloc = temp_alloc_wrap,
        .realloc = temp_realloc_wrap,
        .free = temp_free_wrap,
    },
    .start = temp_mem,
    .end = temp_mem + EXT_DEAFULT_TEMP_SIZE,
    .mem_size = EXT_DEAFULT_TEMP_SIZE,
    .mem = temp_mem,
};

static void *temp_alloc_wrap(Ext_Allocator *a, size_t size) {
    (void)a;
    return ext_temp_alloc(size);
}

static void *temp_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    (void)a;
    return ext_temp_realloc(ptr, old_size, new_size);
}

static void temp_free_wrap(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)ptr;
    (void)size;
    // No-op, temp allocator does not free memory
}

void ext_temp_set_mem(void *mem, size_t size) {
    ext_temp_allocator.mem_size = size;
    ext_temp_allocator.mem = mem;
    ext_temp_reset();
}

void *ext_temp_alloc(size_t size) {
    ptrdiff_t alignment = -size & (EXT_DEFAULT_ALIGN - 1);
    ptrdiff_t available = ext_temp_allocator.end - ext_temp_allocator.start - alignment;
    if((ptrdiff_t)size > available) {
        fprintf(stderr, "%s:%d: temp allocation failed: %zu bytes requested, %zu bytes available\n",
                __FILE__, __LINE__, size, available);
        abort();
    }
    void *p = ext_temp_allocator.start;
    ext_temp_allocator.start += size + alignment;
    return p;
}

void *ext_temp_realloc(void *ptr, size_t old_size, size_t new_size) {
    ptrdiff_t alignment = -(uintptr_t)old_size & (EXT_DEFAULT_ALIGN - 1);
    // Reallocating last allocated memory, can grow/shrink in-place
    if(ext_temp_allocator.start - old_size - alignment == ptr) {
        ext_temp_allocator.start -= old_size + alignment;
        return ext_temp_alloc(new_size);
    } else if(new_size > old_size) {
        void *new_ptr = ext_temp_alloc(new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    } else {
        return ptr;
    }
}

size_t ext_temp_available(void) {
    return ext_temp_allocator.end - ext_temp_allocator.start;
}

void ext_temp_reset(void) {
    ext_temp_allocator.start = ext_temp_allocator.mem;
    ext_temp_allocator.end = ext_temp_allocator.mem + ext_temp_allocator.mem_size;
}

void *ext_temp_checkpoint(void) {
    return ext_temp_allocator.start;
}

void ext_temp_rewind(void *checkpoint) {
    ext_temp_allocator.start = checkpoint;
}

char *ext_temp_strdup(const char *str) {
    size_t n = strlen(str);
    char *res = ext_temp_alloc(n + 1);
    memcpy(res, str, n);
    res[n] = '\0';
    return res;
}

char *ext_temp_sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *res = ext_temp_vsprintf(fmt, ap);
    va_end(ap);
    return res;
}

char *ext_temp_vsprintf(const char *fmt, va_list ap) {
    va_list cpy;
    va_copy(cpy, ap);
    int n = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);

    assert(n >= 0 && "error in vsnprintf");
    char *res = ext_temp_alloc(n + 1);

    va_copy(cpy, ap);
    vsnprintf(res, n + 1, fmt, cpy);
    va_end(cpy);

    return res;
}
