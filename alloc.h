#ifndef ALLOC_H
#define ALLOC_H

// TODO: support custom alignment and implement growable arena allocator
//         - For this, it would probably be good to overload a macro
// TODO: thread safety with thread-local storage
// TODO: document this shit

#include <stddef.h>

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
extern Ext_Allocator *ext_allocator_ctx;

void ext_push_allocator(Ext_Allocator *alloc);
Ext_Allocator *ext_pop_allocator(void);

// -----------------------------------------------------------------------------
// Temp allocator API
//

typedef struct Ext_TempAllocator {
    Ext_Allocator base;
    char *start, *end;
} Ext_TempAllocator;
extern Ext_TempAllocator ext_temp_allocator;

// TODO: probably better to put this in a 'config.h' header
#ifndef EXT_ALLOC_TEMP_SIZE
    #define EXT_ALLOC_TEMP_SIZE (64 * 1024)  // 64 KiB
#endif

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
    #define temp_reset          ext_temp_reset
    #define push_temp_allocator ext_push_temp_allocator
#endif

#endif
