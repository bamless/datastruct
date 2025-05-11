#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "alloc.h"
#include "hashmap.h"
#include "array.h"
#include "context.h"

#define THREAD_TMP_SIZE (256 * 1024 * 1024)

typedef struct {
    int key;
    int value;
} IntEntry;

typedef struct {
    IntEntry *entries;
    size_t *hashes;
    size_t size, capacity;
    Ext_Allocator *allocator;
} IntHashMap;

typedef struct {
    int *items;
    size_t size, capacity;
    Ext_Allocator *allocator;
} IntArray;

static void ds_test(void) {
    printf("HashMap ----------------------------\n");

    IntHashMap map = {0};
    for(int i = 0; i < 22; i++) {
        IntEntry e = {.key = i, .value = i * 10};
        hmap_put(&map, &e);
    }

    printf("number of entries: %zu\n", map.size);
    hmap_put(&map, &((IntEntry){.key = 2, .value = 100}));

    IntEntry *entry;
    hmap_get(&map, &((IntEntry){.key = 2}), &entry);
    assert(entry != NULL);
    printf("Key: %d, Value: %d\n", entry->key, entry->value);
    entry->value += 50;

    hmap_delete(&map, &((IntEntry){.key = 10}));
    hmap_get(&map, &((IntEntry){.key = 10}), &entry);
    assert(entry == NULL);

    hmap_get(&map, &((IntEntry){.key = 100}), &entry);
    assert(entry == NULL);

    hmap_get(&map, &((IntEntry){.key = 3}), &entry);
    assert(entry != NULL);
    printf("Key: %d, Value: %d\n", entry->key, entry->value);

    hmap_get(&map, &((IntEntry){.key = 2}), &entry);
    assert(entry != NULL);
    assert(entry->value == 150);
    printf("Key: %d, Value: %d\n", entry->key, entry->value);

    printf("number of entries: %zu\n", map.size);

    printf("HashMap contents:\n");
    hmap_foreach(IntEntry, e, &map) {
        printf("Key: %d, Value: %d\n", e->key, e->value);
    }

    printf("Array ----------------------------\n");

    IntArray arr = {0};
    for(int i = 0; i < 200; i++) {
        array_push(&arr, i);
    }

    array_foreach(int, elem, &arr) {
        printf("%d ", *elem);
    }
    printf("\n");
}

static int t2_start(void* data) {
    (void)data;
    void* temp = ext_alloc(THREAD_TMP_SIZE);
    ext_temp_set_mem(temp, THREAD_TMP_SIZE);

    Ext_Context ctx = *ext_context;
    ctx.alloc = &ext_temp_allocator.base;
    ext_push_context(&ctx);

    while(1) {
        ds_test();
        ext_temp_reset();
    }

    ext_pop_context();
    ext_free(temp, THREAD_TMP_SIZE);
    return 0;
}

static int t1_start(void* data) {
    (void)data;
    void* temp = ext_alloc(THREAD_TMP_SIZE);
    ext_temp_set_mem(temp, THREAD_TMP_SIZE);

    Ext_Context ctx = *ext_context;
    ctx.alloc = &ext_temp_allocator.base;
    ext_push_context(&ctx);

    while(1) {
        ds_test();
        ext_temp_reset();
    }

    ext_pop_context();
    ext_free(temp, THREAD_TMP_SIZE);
    return 0;
}

int main(void) {
    thrd_t t1;
    if(thrd_create(&t1, t1_start, NULL) != thrd_success) {
        fprintf(stderr, "Couldn't create thread 1");
        abort();
    }

    thrd_t t2;
    if(thrd_create(&t2, t2_start, NULL)) {
        fprintf(stderr, "Couldn't create thread 2");
        abort();
    }

    if(thrd_join(t1, NULL)) {
        fprintf(stderr, "Couldn't join thread 1");
        abort();
    }

    if(thrd_join(t2, NULL)) {
        fprintf(stderr, "Couldn't join thread 2");
        abort();
    }
}
