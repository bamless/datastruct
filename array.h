#ifndef ARRAY_H
#define ARRAY_H

#include <assert.h>

#include "alloc.h"  // IWYU pragma: keep

#ifndef ARRAY_INIT_CAPACITY
    #define ARRAY_INIT_CAPACITY 4
#endif

#define EXT_ARRAY_ITEMS(T) \
    T* items;              \
    size_t size, capacity; \
    Ext_Allocator* allocator;

// Utility macro for iterating in a foreach style
#define array_foreach(T, it, vec) \
    for(T* it = (vec)->items, *end = (vec)->items + (vec)->size; it != end; it++)

#define array_reserve(arr, newcap)                                                         \
    do {                                                                                   \
        if((arr)->capacity < (newcap)) {                                                   \
            size_t oldcap = (arr)->capacity;                                               \
            (arr)->capacity = oldcap ? (arr)->capacity * 2 : ARRAY_INIT_CAPACITY;          \
            while((arr)->capacity < (newcap)) {                                            \
                (arr)->capacity <<= 1;                                                     \
            }                                                                              \
            if(!((arr)->allocator)) {                                                      \
                (arr)->allocator = ext_allocator_ctx;                                      \
            }                                                                              \
            Ext_Allocator* a = (arr)->allocator;                                           \
            if(!(arr)->items) {                                                            \
                (arr)->items = a->alloc(a, (arr)->capacity * sizeof(*(arr)->items));       \
            } else {                                                                       \
                (arr)->items = a->realloc(a, (arr)->items, oldcap * sizeof(*(arr)->items), \
                                          (arr)->capacity * sizeof(*(arr)->items));        \
            }                                                                              \
        }                                                                                  \
    } while(0)

#define array_push(a, v)                   \
    do {                                   \
        array_reserve((a), (a)->size + 1); \
        (a)->items[(a)->size++] = (v);     \
    } while(0)

#define array_free(a)                                                                              \
    do {                                                                                           \
        if((a)->allocator) {                                                                       \
            (a)->allocator->free((a)->allocator, (a)->items, (a)->capacity * sizeof(*(a)->items)); \
        }                                                                                          \
        memset((a), 0, sizeof(*(a)));                                                              \
    } while(0)

#ifndef EXTLIB_NO_SHORTHANDS
    #define ARRAY_ITEMS EXT_ARRAY_ITEMS
#endif

#endif
