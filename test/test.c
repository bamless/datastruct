#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

CTEST(alloc, alloc_free) {
    int* i = ext_alloc(sizeof(int));
    ASSERT_TRUE(allocated == sizeof(int));
    i = ext_realloc(i, sizeof(int), sizeof(int) * 20);
    ASSERT_TRUE(allocated == sizeof(int) * 20);
    ext_free(i, sizeof(int) * 20);
    ASSERT_TRUE(allocated == 0);
}

CTEST(alloc, new_delete) {
    int* i = ext_new(int);
    ASSERT_TRUE(allocated == sizeof(int));
    ext_delete(int, i);
    ASSERT_TRUE(allocated == 0);

    int* ints = ext_new_array(int, 20);
    ASSERT_TRUE(allocated == 20 * sizeof(int));
    ext_delete_array(int, 20, ints);
    ASSERT_TRUE(allocated == 0);
}

CTEST(temp, set_mem) {
    void* new_mem = malloc(1000);
    temp_set_mem(new_mem, 1000);
    ASSERT_TRUE(ext_temp_allocator.start == new_mem);
    ASSERT_TRUE(ext_temp_allocator.mem_size == 1000);
    short* i = temp_alloc(sizeof(int));
    *i = 49;
    ASSERT_TRUE(*i == 49);
    temp_set_mem(ext_temp_mem, sizeof(ext_temp_mem));
    ASSERT_TRUE(ext_temp_allocator.mem == ext_temp_mem);
    ASSERT_TRUE(ext_temp_allocator.mem_size == sizeof(ext_temp_mem));
    free(new_mem);
}

CTEST(temp, reset) {
    temp_alloc(1000);
    ASSERT_TRUE(ext_temp_allocator.start > (char*)ext_temp_allocator.mem);
    temp_reset();
    ASSERT_TRUE(ext_temp_allocator.start == ext_temp_allocator.mem);
}

CTEST(temp, alloc) {
    int* i = temp_alloc(sizeof(*i));
    ASSERT_TRUE(ext_temp_allocator.start - (char*)ext_temp_allocator.mem >= (intptr_t)sizeof(int));
    temp_reset();
}

CTEST(temp, realloc) {
    int* ints = temp_alloc(sizeof(int) * 100);
    for(int i = 0; i < 100; i++) {
        ints[i] = i;
    }
    int* new_ints = temp_realloc(ints, 100 * sizeof(int), 1000 * sizeof(int));
    ASSERT_TRUE(ints == new_ints);
    for(int i = 0; i < 100; i++) {
        ASSERT_TRUE(new_ints[i] == i);
    }
    for(int i = 100; i < 1000; i++) {
        ints[i] = i;
    }
    temp_alloc(sizeof(int));
    new_ints = temp_realloc(ints, 1000 * sizeof(int), 10000 * sizeof(int));
    ASSERT_TRUE(ints != new_ints);
    for(int i = 0; i < 1000; i++) {
        ASSERT_TRUE(new_ints[i] == i);
    }
    temp_reset();
}

CTEST(temp, available) {
    temp_alloc(sizeof(int));
    ASSERT_TRUE(temp_available() <= ext_temp_allocator.mem_size - sizeof(int));
    temp_reset();
}

CTEST(temp, checkpoint) {
    temp_alloc(100 * sizeof(int));
    char* start = ext_temp_allocator.start;
    void* checkpoint = temp_checkpoint();
    temp_alloc(1000 * sizeof(int));
    temp_rewind(checkpoint);
    ASSERT_TRUE(ext_temp_allocator.start == start);
    temp_reset();
}

CTEST(temp, strdup) {
    const char* str = "Cantami, o Diva, del Pelide Achille";
    char* dup = temp_strdup(str);
    ASSERT_TRUE(str != dup && strcmp(str, dup) == 0);
    temp_reset();
}

CTEST(temp, memdup) {
    unsigned char mem[] = "Cantami,\0o\0Diva,\0del\0Pelide\0Achille";
    unsigned char* dup = temp_memdup(mem, sizeof(mem));
    ASSERT_TRUE(mem != dup && memcmp(mem, dup, sizeof(mem)) == 0);
    temp_reset();
}

#ifndef EXTLIB_NO_STD
CTEST(temp, sprintf) {
    char* s = temp_sprintf("%d %s", 3, "ciao");
    ASSERT_TRUE(strcmp(s, "3 ciao") == 0);
    temp_reset();
}
#endif  // EXTLIB_NO_STD

typedef struct {
    Allocator base;
    bool noop;
} NoopAlloc;

void* noop_alloc(Allocator* a, size_t size) {
    (void)size;
    NoopAlloc* allocator = (NoopAlloc*)a;
    ASSERT_TRUE(allocator->noop);
    return (void*)1;
}

void* noop_realloc(Allocator* a, void* ptr, size_t old_sz, size_t new_sz) {
    (void)ptr, (void)old_sz, (void)new_sz;
    NoopAlloc* allocator = (NoopAlloc*)a;
    ASSERT_TRUE(allocator->noop);
    return (void*)2;
}

void noop_free(Allocator* a, void* ptr, size_t size) {
    (void)ptr, (void)size;
    NoopAlloc* allocator = (NoopAlloc*)a;
    ASSERT_TRUE(allocator->noop);
}

NoopAlloc noop_allocator = {{noop_alloc, noop_realloc, noop_free}, true};

CTEST(context, push_pop) {
    Context ctx = *ext_context;
    ctx.alloc = &noop_allocator.base;

    push_context(&ctx);
    {
        ASSERT_TRUE(ext_context == &ctx);
        ASSERT_TRUE(ext_context->alloc->alloc == noop_alloc);
        ASSERT_TRUE(ext_context->prev->alloc == &tracking_allocator);
        int* ptr = ext_alloc(sizeof(int));
        ASSERT_TRUE(ptr == (void*)1);
        ptr = ext_realloc(ptr, sizeof(int), sizeof(int) * 20);
        ASSERT_TRUE(ptr == (void*)2);
        ext_free(ptr, sizeof(int) * 20);
    }
    pop_context();
    ASSERT_TRUE(ext_context != &ctx);
    ASSERT_TRUE(ext_context->alloc == &tracking_allocator);
}

CTEST(arena, alloc_realloc_free) {
    Arena a = new_arena(NULL, 0, 0, 0);
    int* i = arena_alloc(&a, sizeof(int));
    *i = 42;
    ASSERT_TRUE(a.allocated >= sizeof(int));
    ASSERT_TRUE(a.first_page && a.first_page == a.last_page);
    int* new_i = arena_realloc(&a, i, sizeof(int), sizeof(int) * 20);
    ASSERT_TRUE(*new_i == 42);
    ASSERT_TRUE(i == new_i);
    ASSERT_TRUE(a.allocated == sizeof(int) * 20);
    for(int i = 1; i < 20; i++) {
        new_i[i] = i;
    }
    arena_alloc(&a, sizeof(int));
    new_i = arena_realloc(&a, i, sizeof(int) * 20, sizeof(int) * 100);
    ASSERT_TRUE(new_i != i);
    ASSERT_TRUE(*new_i == 42);
    ASSERT_TRUE(a.allocated >= sizeof(int) + sizeof(int) * 20 + sizeof(int) * 100);
    for(int i = 1; i < 20; i++) {
        ASSERT_TRUE(i == new_i[i]);
    }

    char* start = a.last_page->start;
    arena_free(&a, new_i, sizeof(int) * 100);
    ASSERT_TRUE(a.last_page->start < start);

    arena_reset(&a);
    ASSERT_TRUE(a.allocated == 0);
    ASSERT_TRUE(a.first_page != NULL &&
                ((size_t)(a.first_page->end - (char*)a.first_page) == a.page_size));

    arena_destroy(&a);
    ASSERT_TRUE(allocated == 0);
}

CTEST(arena, flexible_page) {
    Arena a = new_arena(NULL, 0, 100, EXT_ARENA_FLEXIBLE_PAGE);
    void* mem = arena_alloc(&a, 1000);
    ASSERT_TRUE(mem != NULL);
    arena_destroy(&a);
}

CTEST(arena, alignment) {
    Arena a = new_arena(NULL, 128, 0, 0);
    int* i = arena_alloc(&a, sizeof(int));
    ASSERT_TRUE((intptr_t)i % 128 == 0);
    ASSERT_TRUE(a.allocated == sizeof(int) + EXT_ALIGN(sizeof(int), 128));
    int* new_i = arena_realloc(&a, i, sizeof(int), sizeof(int) * 10);
    ASSERT_TRUE(i == new_i);
    arena_alloc(&a, 1);
    new_i = arena_realloc(&a, new_i, sizeof(int) * 10, sizeof(int) * 100);
    ASSERT_TRUE(i != new_i);
    ASSERT_TRUE((intptr_t)new_i % 128 == 0);
    arena_reset(&a);
    ASSERT_TRUE(a.allocated == 0);
    arena_destroy(&a);
    ASSERT_TRUE(allocated == 0);
}

CTEST(arena, custom_allocator) {
    Arena a = new_arena(&ext_temp_allocator.base, 0, 0, 0);
    int* i = arena_alloc(&a, sizeof(int));
    ASSERT_TRUE(i != NULL);
    ASSERT_TRUE(ext_temp_allocator.start - (char*)ext_temp_allocator.mem >= (intptr_t)sizeof(int));
    ASSERT_TRUE(allocated == 0);
    arena_destroy(&a);
    temp_reset();
}

CTEST(arena, strdup) {
    Arena a = new_arena(NULL, 0, 0, 0);
    const char* str = "Cantami, o Diva, del Pelide Achille";
    char* dup = arena_strdup(&a, str);
    ASSERT_TRUE(str != dup && strcmp(str, dup) == 0);
    ext_arena_destroy(&a);
}

CTEST(arena, memdup) {
    Arena a = new_arena(NULL, 0, 0, 0);
    unsigned char mem[] = "Cantami,\0o\0Diva,\0del\0Pelide\0Achille";
    unsigned char* dup = arena_memdup(&a, mem, sizeof(mem));
    ASSERT_TRUE(mem != dup && memcmp(mem, dup, sizeof(mem)) == 0);
    arena_destroy(&a);
}

#ifndef EXTLIB_NO_STD
CTEST(arena, sprintf) {
    Arena a = new_arena(NULL, 0, 0, 0);
    char* s = arena_sprintf(&a, "%d %s", 3, "ciao");
    ASSERT_TRUE(strcmp(s, "3 ciao") == 0);
    arena_destroy(&a);
}
#endif  // EXTLIB_NO_STD

CTEST(array, reserve) {
    Ints ints = {0};
    array_reserve(&ints, 100);
    ASSERT_TRUE(ints.capacity >= 100);
    size_t cur_capacity = ints.capacity;
    array_reserve(&ints, 5);
    ASSERT_TRUE(ints.capacity == cur_capacity);
    array_free(&ints);
    ASSERT_TRUE(allocated == 0);
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
    ASSERT_TRUE(allocated == 0);
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
    ASSERT_TRUE(allocated == 0);

    pop_context();
    temp_reset();
}

CTEST(sb, append) {
    const char s[] = "Cantami,\0o\0Diva,\0del\0Pelide\0Achille";
    StringBuffer sb = {0};
    sb_append(&sb, s, sizeof(s) - 1);
    ASSERT_TRUE(sb.size == sizeof(s) - 1);
    ASSERT_TRUE(memcmp(s, sb.items, sizeof(s) - 1) == 0);
    sb_free(&sb);
}

CTEST(sb, append_cstr) {
    const char s[] = "Cantami, o Diva, del Pelide Achille";
    StringBuffer sb = {0};
    sb_append_cstr(&sb, s);
    ASSERT_TRUE(sb.size == sizeof(s) - 1);
    ASSERT_TRUE(memcmp(s, sb.items, sizeof(s) - 1) == 0);
    sb_free(&sb);
}

CTEST(sb, to_cstr) {
    StringBuffer sb = {0};
    const char s[] = "Cantami, o Diva, del Pelide Achille";
    sb_append(&sb, s, sizeof(s) - 1);
    char* res = sb_to_cstr(&sb);
    ASSERT_TRUE(strcmp(s, res) == 0);
    ext_free(res, strlen(res) + 1);
}

#ifndef EXTLIB_NO_STD
CTEST(sb, appendf) {
    StringBuffer sb = {0};
    int res = sb_appendf(&sb, "%s:%d", "test.c", 494);
    const char* expected = "test.c:429";
    ASSERT_TRUE(res == (int)strlen(expected));
    ASSERT_TRUE(memcmp(expected, sb.items, strlen(expected)));
    sb_free(&sb);
}
#endif

typedef struct {
    int key;
    int value;
} IntEntry;

typedef struct {
    IntEntry* entries;
    size_t* hashes;
    size_t size, capacity;
    Allocator* allocator;
} IntMap;

CTEST(hmap, get_put) {
    IntMap map = {0};
    for(int i = 0; i < 22; i++) {
        hmap_put(&map, &((IntEntry){.key = i, .value = i * 10}));
    }

    ASSERT_TRUE(map.size == 22);
    hmap_put(&map, &((IntEntry){.key = 2, .value = 100}));
    ASSERT_TRUE(map.size == 22);

    IntEntry* entry;
    hmap_get(&map, &((IntEntry){.key = 2}), &entry);
    ASSERT_TRUE(entry != NULL);
    ASSERT_TRUE(entry->value = 100);
    entry->value += 50;

    IntEntry* entry2;
    hmap_get(&map, &((IntEntry){.key = 2}), &entry2);
    ASSERT_TRUE(entry == entry2);
    ASSERT_TRUE(entry2->value == 150);

    hmap_free(&map);
}

CTEST(hmap, delete) {
    IntEntry* e;
    IntMap map = {0};

    for(int i = 1; i < 50; i++) {
        hmap_put(&map, &((IntEntry){.key = i, .value = i * 10}));
    }

    hmap_get(&map, &((IntEntry){.key = 1}), &e);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->value == 10);

    ASSERT_TRUE(map.size == 49);
    hmap_delete(&map, &((IntEntry){.key = 1}));
    ASSERT_TRUE(map.size == 48);
    hmap_get(&map, &((IntEntry){.key = 1}), &e);
    ASSERT_TRUE(e == NULL);

    for(int i = 2; i < 50; i++) {
        hmap_delete(&map, &((IntEntry){.key = i}));
    }
    for(int i = 2; i < 50; i++) {
        IntEntry* e;
        hmap_get(&map, &((IntEntry){.key = i}), &e);
        ASSERT_TRUE(e == NULL);
    }
    ASSERT_TRUE(map.size == 0);

    hmap_free(&map);
}

CTEST(hmap, clear) {
    IntMap map = {0};
    for(int i = 0; i < 10; i++) {
        hmap_put(&map, &((IntEntry){.key = i, .value = i * 10}));
    }
    ASSERT_TRUE(map.size == 10);
    hmap_clear(&map);
    ASSERT_TRUE(map.size == 0);
    for(int i = 0; i < 10; i++) {
        IntEntry* e;
        hmap_get(&map, &((IntEntry){.key = i}), &e);
        ASSERT_TRUE(e == NULL);
    }

    hmap_put(&map, &((IntEntry){.key = 3, .value = 100}));
    ASSERT_TRUE(map.size == 1);

    IntEntry* e;
    hmap_get(&map, &((IntEntry){.key = 3}), &e);
    ASSERT_TRUE(e != NULL);
    ASSERT_TRUE(e->key == 3 && e->value == 100);

    hmap_clear(&map);
    ASSERT_TRUE(map.size == 0);

    hmap_free(&map);
}

CTEST(hmap, iter) {
    IntMap map = {0};
    for(int i = 0; i < 50; i++) {
        hmap_put(&map, &((IntEntry){.key = i, .value = i * 10}));
    }
    int i = 0;
    for(IntEntry* it = hmap_begin(&map); it != hmap_end(&map); it = hmap_next(&map, it)) {
        i++;
    }
    ASSERT_TRUE(i == 50);

    i = 0;
    hmap_foreach(IntEntry, it, &map) {
        i++;
    }
    ASSERT_TRUE(i == 50);

    hmap_free(&map);
}

CTEST(hmap, allocator) {
    IntMap ints = {0};
    size_t temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail == 0);
    ints.allocator = &ext_temp_allocator.base;
    for(int i = 0; i < 100; i++) {
        hmap_put(&ints, &((IntEntry){.key = i, .value = i * 10}));
    }
    temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail >= 100 * sizeof(IntEntry));
    ASSERT_TRUE(allocated == 0);
    temp_reset();
}

CTEST(hmap, ctx_allocator) {
    Context ctx = *ext_context;
    ctx.alloc = &ext_temp_allocator.base;
    push_context(&ctx);

    IntMap ints = {0};
    size_t temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail == 0);
    ints.allocator = &ext_temp_allocator.base;
    for(int i = 0; i < 100; i++) {
        hmap_put(&ints, &((IntEntry){.key = i, .value = i * 10}));
    }
    temp_avail = ext_temp_allocator.end - ext_temp_allocator.start;
    ASSERT_TRUE(ext_temp_allocator.mem_size - temp_avail >= 100 * sizeof(IntEntry));
    ASSERT_TRUE(allocated == 0);

    pop_context();
    temp_reset();
}
