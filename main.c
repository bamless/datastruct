#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EXTLIB_IMPL
#include "extlib.h"

typedef struct {
    int key;
    int value;
} IntEntry;

typedef struct {
    IntEntry *entries;
    size_t *hashes;
    size_t size, capacity;
    Allocator *allocator;
} IntHashMap;

typedef struct {
    int *items;
    size_t size, capacity;
    Allocator *allocator;
} IntArray;

int main(void) {
    Arena a = new_arena(NULL, 0, 0, EXT_ARENA_FLEXIBLE_PAGE);
    Context ctx = *ext_context;
    ctx.alloc = &a.base;
    push_context(&ctx);

    char *res = ext_temp_sprintf("This is an int: %d\n", 3);
    printf("%s\n", res);

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
    arena_reset(&a);

    IntArray arr = {0};
    for(int i = 0; i < 1000; i++) {
        array_push(&arr, i);
    }

    array_foreach(int, elem, &arr) {
        printf("%d ", *elem);
    }
    printf("\n");

    pop_context();
    arena_destroy(&a);

    return 0;
}
