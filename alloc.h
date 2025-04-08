#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>

#ifndef EXTLIB_NO_ALLOC_THREAD_SAFE
    #if defined(_MSC_VER)
        // MSVC supports __declspec(thread), but not for dynamically initialized data
        #define EXT_TLS __declspec(thread)
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && \
        !defined(__STDC_NO_THREADS__)
        #include <threads.h>
        #define EXT_TLS thread_local
    #elif defined(__GNUC__) || defined(__clang__)
        // GCC and Clang (even with C99 or C89)
        #define EXT_TLS __thread
    #else
        #warning \
            "thread local is not supported on this compiler. Fallback to global (non-thread-safe) storage."
        #define EXT_TLS
    #endif
#else
    #define EXT_TLS
#endif

// -----------------------------------------------------------------------------
// Allocator API
//

#define ext_allocate(size) ext_allocator_ctx->alloc(ext_allocator_ctx, size)
#define ext_reallocate(ptr, old_size, new_size) \
    ext_allocator_ctx->realloc(ext_allocator_ctx, ptr, old_size, new_size)
#define ext_deallocate(ptr, size) ext_allocator_ctx->free(ext_allocator_ctx, ptr, size)

typedef struct Ext_Allocator {
    void *(*alloc)(struct Ext_Allocator *, size_t size);
    void *(*realloc)(struct Ext_Allocator *, void *ptr, size_t old_size, size_t new_size);
    void (*free)(struct Ext_Allocator *, void *ptr, size_t size);
    struct Ext_Allocator *prev;
} Ext_Allocator;
extern EXT_TLS Ext_Allocator *ext_allocator_ctx;

void ext_push_allocator(Ext_Allocator *alloc);
Ext_Allocator *ext_pop_allocator(void);

// -----------------------------------------------------------------------------
// Default allocator
//

typedef struct Ext_DefaultAllocator {
    Ext_Allocator base;
} Ext_DefaultAllocator;
extern EXT_TLS const Ext_DefaultAllocator ext_default_allocator;

// -----------------------------------------------------------------------------
// Temp allocator API
//

typedef struct Ext_TempAllocator {
    Ext_Allocator base;
    char *start, *end;
} Ext_TempAllocator;
extern EXT_TLS Ext_TempAllocator ext_temp_allocator;

void *ext_temp_allocate(size_t size);
void *ext_temp_reallocate(void *ptr, size_t old_size, size_t new_size);
size_t ext_temp_available(void);
void ext_temp_reset(void);
void ext_push_temp_allocator(void);

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_Allocator Allocator;
    #define allocator_ctx ext_allocator_ctx

    #define allocate   ext_allocate
    #define reallocate ext_reallocate
    #define deallocate ext_deallocate

    #define push_allocator ext_push_allocator
    #define pop_allocator  ext_pop_allocator

typedef Ext_TempAllocator TempAllocator;
    #define temp_allocator      ext_temp_allocator
    #define temp_allocate       ext_temp_allocate
    #define temp_reallocate     ext_temp_reallocate
    #define temp_available      ext_temp_available
    #define temp_reset          ext_temp_reset
    #define push_temp_allocator ext_push_temp_allocator
#endif

#endif
