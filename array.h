#ifndef ARRAY_H
#define ARRAY_H

#include <assert.h>
#include <stdlib.h>

#ifndef ARRAY_INIT_CAPACITY
    #define ARRAY_INIT_CAPACITY 4
#endif

// Utility macro for iterating in a foreach style
#define array_foreach(elem, vec)                                                          \
    for(size_t __cont = 1, __i = 0; __cont && __i < (vec)->size; __cont = !__cont, __i++) \
        for(elem = (vec)->data + __i; __cont; __cont = !__cont)

#define REALLOC(data, size) realloc(data, size)
#define FREE(data)          free(data)

#define array_reserve(a, newcap)                                                     \
    do {                                                                             \
        if((a)->capacity < (newcap)) {                                               \
            (a)->capacity = (a)->capacity ? (a)->capacity * 2 : ARRAY_INIT_CAPACITY; \
            while((a)->capacity < (newcap)) {                                        \
                (a)->capacity <<= 1;                                                 \
            }                                                                        \
            (a)->data = REALLOC((a)->data, (a)->capacity * sizeof(*(a)->data));      \
            assert((a)->data && "Out of memory");                                    \
        }                                                                            \
    } while(0)

#define array_push(a, v)                   \
    do {                                   \
        array_reserve((a), (a)->size + 1); \
        (a)->data[(a)->size++] = (v);      \
    } while(0)

#define array_free(a)      \
    do {                   \
        FREE((a)->data);   \
        (a)->data = NULL;  \
        (a)->size = 0;     \
        (a)->capacity = 0; \
    } while(0)

#endif
