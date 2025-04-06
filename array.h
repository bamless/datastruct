#ifndef ARRAY_H
#define ARRAY_H

#include <assert.h>

#include "alloc.h"

#ifndef ARRAY_INIT_CAPACITY
    #define ARRAY_INIT_CAPACITY 4
#endif

// Utility macro for iterating in a foreach style
#define array_foreach(T, it, vec) \
    for(T *it = (vec)->data, *end = (vec)->data + (vec)->size; it != end; it++)

#define array_reserve(a, newcap)                                                          \
    do {                                                                                  \
        if((a)->capacity < (newcap)) {                                                    \
            (a)->capacity = (a)->capacity ? (a)->capacity * 2 : ARRAY_INIT_CAPACITY;      \
            while((a)->capacity < (newcap)) {                                             \
                (a)->capacity <<= 1;                                                      \
            }                                                                             \
            if(!(a)->data) {                                                              \
                (a)->data = ext_allocate((a)->capacity * sizeof(*(a)->data));             \
            } else {                                                                      \
                (a)->data = ext_reallocate((a)->data, (a)->capacity * sizeof(*(a)->data), \
                                           (newcap) * sizeof(*(a)->data));                \
            }                                                                             \
        }                                                                                 \
    } while(0)

#define array_push(a, v)                   \
    do {                                   \
        array_reserve((a), (a)->size + 1); \
        (a)->data[(a)->size++] = (v);      \
    } while(0)

#define array_free(a)                                              \
    do {                                                           \
        ext_deallocate((a)->data, (a)->capacity * sizeof(*(a)->data)); \
        memset((a), 0, sizeof(*(a)));                              \
    } while(0)

#endif
