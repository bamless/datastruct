#ifndef ALLOC_H
#define ALLOC_H

#include <stdarg.h>
#include <stddef.h>

#include "context.h"  // IWYU pragma: export

// -----------------------------------------------------------------------------
// Allocator API
//

#define ext_new(T)         ext_allocate(sizeof(T))
#define ext_delete(T, ptr) ext_deallocate(ptr, sizeof(T))

#define ext_allocate(size) ext_context->alloc->allocate(ext_context->alloc, size)
#define ext_reallocate(ptr, old_size, new_size) \
    ext_context->alloc->realloc(ext_context->alloc, ptr, old_size, new_size)
#define ext_deallocate(ptr, size) ext_context->alloc->free(ext_context->alloc, ptr, size)

typedef struct Ext_Allocator {
    void *(*allocate)(struct Ext_Allocator *, size_t size);
    void *(*reallocate)(struct Ext_Allocator *, void *ptr, size_t old_size, size_t new_size);
    void (*deallocate)(struct Ext_Allocator *, void *ptr, size_t size);
} Ext_Allocator;

// -----------------------------------------------------------------------------
// Default allocator
//

typedef struct Ext_DefaultAllocator {
    Ext_Allocator base;
} Ext_DefaultAllocator;
extern Ext_DefaultAllocator ext_default_allocator;

// -----------------------------------------------------------------------------
// Temp allocator
//

typedef struct Ext_TempAllocator {
    Ext_Allocator base;
    char *start, *end;
} Ext_TempAllocator;
extern Ext_TempAllocator ext_temp_allocator;

void *ext_temp_allocate(size_t size);
void *ext_temp_reallocate(void *ptr, size_t old_size, size_t new_size);
size_t ext_temp_available(void);
void ext_temp_reset(void);
void *ext_temp_checkpoint(void);
void ext_temp_rewind(void *checkpoint);
char* ext_temp_strdup(const char* str);
char* ext_temp_sprintf(const char* fmt, ...);
char* ext_temp_vsprintf(const char* fmt, va_list ap);

#endif
