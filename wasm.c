#define EXTLIB_IMPL
#include "extlib.h"

typedef struct {
    int* items;
    size_t capacity, size;
    Allocator* allocator;
} IntArray;

/* Pointless function to show how to work with extlib datastruture from within wasm. Here we
 * use the temp allocator */
int* make_array(int n) {
    assert(n > 0);
    IntArray arr = {0};
    arr.allocator = &ext_temp_allocator.base;

    for(int i = 0; i < n; i++) {
        array_push(&arr, i);
    }

    return arr.items;
}

/* Instead of explicitely setting the allocator as above, it is also possible to push a context
 * allocator that the program will use from then on */
int* make_array_ctx(int n) {
    assert(n > 0);

    Context ctx = *ext_context;
    ctx.alloc = &ext_temp_allocator.base;
    push_context(&ctx);

    IntArray arr = {0};
    for(int i = 0; i < n; i++) {
        array_push(&arr, i);
    }

    pop_context();
    return arr.items;
}

/* Clearly, it is possible to implement your own allocator that works with `__heap_base` and does
 * all the fancy stuff it wants (or uses some already implemented one, like
 * https://github.com/rustwasm/wee_alloc).
 *
 * By default, extlib's default allocator in wasm simply takes memory from the top of `__heap_base`
 * and never returns it to the system.
 * A simple strategy for working with this allocation scheme without resorting in writing your own
 * is allocating arenas from it
 */
int* make_array_arena(int n) {
    // Create an arena that uses the current context allocator, in this case `default`.
    // this is probably better allocated outside, so we can `clear` it whe  we are done with the
    // memory and reused
    Arena a = new_arena(NULL, 0, 0, 0);

    Context ctx = *ext_context;
    ctx.alloc = &a.base;
    push_context(&ctx);

    IntArray arr = {0};
    for(int i = 0; i < n; i++) {
        array_push(&arr, i);
    }

    pop_context();
    return arr.items;
}
