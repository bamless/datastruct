#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#undef __STRICT_ANSI__
#define CTEST_MAIN
#define CTEST_SEGFAULT
#define CTEST_COLOR_OK
#include "ctest.h"
#define EXTLIB_IMPL
#include "../extlib.h"

size_t allocated;

void* tracking_alloc(Allocator* a, size_t size) {
    (void)a;
    allocated += size;
    return ext_context->prev->alloc->alloc(ext_context->prev->alloc, size);
}

void* tracking_realloc(Allocator* a, void* ptr, size_t old_sz, size_t new_sz) {
    (void)a;
    allocated += (int)new_sz - (int)old_sz;
    return ext_context->prev->alloc->realloc(ext_context->prev->alloc, ptr, old_sz, new_sz);
}

void tracking_free(Allocator* a, void* ptr, size_t size) {
    (void)a;
    allocated -= size;
    ext_context->prev->alloc->free(ext_context->prev->alloc, ptr, size);
}

Allocator tracking_allocator = {
    tracking_alloc,
    tracking_realloc,
    tracking_free,
};

int main(int argc, const char** argv) {
    Context ctx = *ext_context;
    ctx.alloc = &tracking_allocator;
    push_context(&ctx);
    int ret = ctest_main(argc, argv);
    if(allocated != 0) {
        fprintf(stderr, "\n%s:%d: error: got un-freed data: %zu", __FILE__, __LINE__, allocated);
        return 1;
    }
    pop_context();
    return ret;
}

typedef struct {
    int* items;
    size_t size, capacity;
    Allocator* allocator;
} Ints;

CTEST(array, reserve) {
    Ints ints = {0};
    array_reserve(&ints, 100);
    ASSERT_TRUE(ints.capacity >= 100);
    size_t cur_capacity = ints.capacity;
    array_reserve(&ints, 5);
    ASSERT_TRUE(ints.capacity == cur_capacity);
    array_free(&ints);
}

CTEST(array, reserve_exact) {
    Ints ints = {0};
    array_reserve_exact(&ints, 100);
    ASSERT_TRUE(ints.capacity == 100);
    array_reserve_exact(&ints, 5);
    ASSERT_TRUE(ints.capacity == 100);
    array_free(&ints);
}

CTEST(array, push) {
    Ints ints = {0};
    for(int i = 0; i < 100; i++) {
        array_push(&ints, i);
    }
    for(int i = 0; i < (int)ints.size; i++) {
        ASSERT_TRUE(ints.items[i] == i);
    }
    array_free(&ints);
}

CTEST(array, foreach) {
    Ints ints = {0};
    for(int i = 0; i < 100; i++) {
        array_push(&ints, i);
    }
    int i = 0;
    array_foreach(int, it, &ints) {
        ASSERT_TRUE(*it == i);
        i++;
    }
    array_free(&ints);
}

CTEST(array, push_all) {
    Ints ints = {0};
    int items[] = {0, 1, 2, 3, 4, 5};
    array_push_all(&ints, items, EXT_ARR_SIZE(items));
    ASSERT_TRUE(ints.size == EXT_ARR_SIZE(items));
    for(int i = 0; i < (int)ints.size; i++) {
        ASSERT_TRUE(ints.items[i] == i);
    }
    array_free(&ints);
}

CTEST(array, pop) {
    Ints ints = {0};
    array_push(&ints, 1);
    array_push(&ints, 2);
    ASSERT_TRUE(ints.size == 2);
    ASSERT_TRUE(ints.items[ints.size - 1] == 2);
    int elem = array_pop(&ints);
    ASSERT_TRUE(elem == 2);
    ASSERT_TRUE(ints.size == 1);
    elem = array_pop(&ints);
    ASSERT_TRUE(elem == 1);
    ASSERT_TRUE(ints.size == 0);
    array_free(&ints);
}

CTEST(array, remove) {
    Ints ints = {0};
    int items[] = {1, 2, 3, 4, 5};
    array_push_all(&ints, items, EXT_ARR_SIZE(items));
    array_remove(&ints, 2);
    ASSERT_TRUE(ints.items[2] == 4);
    array_remove(&ints, 0);
    ASSERT_TRUE(ints.items[0] == 2);
    array_remove(&ints, ints.size - 1);
    ASSERT_TRUE(ints.items[ints.size - 1] == 4);
    ASSERT_TRUE(ints.size == 2 && ints.items[0] == 2 && ints.items[1] == 4);
    array_free(&ints);
}

CTEST(array, swap_remove) {
    Ints ints = {0};
    int items[] = {1, 2, 3, 4, 5};
    array_push_all(&ints, items, EXT_ARR_SIZE(items));
    array_swap_remove(&ints, 2);
    ASSERT_TRUE(ints.items[2] == 5);
    array_swap_remove(&ints, 0);
    ASSERT_TRUE(ints.items[0] == 4);
    array_swap_remove(&ints, ints.size - 1);
    ASSERT_TRUE(ints.items[ints.size - 1] == 2);
    ASSERT_TRUE(ints.size == 2 && ints.items[0] == 4 && ints.items[1] == 2);
    array_free(&ints);
}

CTEST(array, clear) {
    Ints ints = {0};
    array_push(&ints, 1);
    array_push(&ints, 2);
    ASSERT_TRUE(ints.size == 2);
    array_clear(&ints);
    ASSERT_TRUE(ints.size == 0 && ints.capacity > 0);
    array_free(&ints);
}

CTEST(array, resize) {
    Ints ints = {0};
    array_resize(&ints, 100);
    ASSERT_TRUE(ints.size == 100);
    array_resize(&ints, 10);
    ASSERT_TRUE(ints.size == 10);
    array_free(&ints);
}

CTEST(array, shrink_to_fit) {
    Ints ints = {0};
    array_push(&ints, 1);
    array_push(&ints, 2);
    ASSERT_TRUE(ints.capacity > 2);
    array_shrink_to_fit(&ints);
    ASSERT_TRUE(ints.capacity == 2);
    array_pop(&ints);
    array_pop(&ints);
    array_shrink_to_fit(&ints);
    ASSERT_TRUE(ints.size == 0 && ints.capacity == 0 && ints.items == NULL);
    array_free(&ints);
}

CTEST(array, allocator) {
    Ints ints = {0};
    size_t temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail == 0);
    ints.allocator = &ext_temp_allocator.base;
    for(int i = 0; i < 100; i++) {
        array_push(&ints, i);
    }
    temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail >= 100 * sizeof(int));
    temp_reset();
}

CTEST(array, ctx_allocator) {
    Context ctx = *ext_context;
    ctx.alloc = &ext_temp_allocator.base;
    push_context(&ctx);

    Ints ints = {0};
    size_t temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail == 0);
    for(int i = 0; i < 100; i++) {
        array_push(&ints, i);
    }
    temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail >= 100 * sizeof(int));

    pop_context();
    temp_reset();
}
