#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "alloc.h"
#include "arena.h"
#include "array.h"
#include "context.h"
#include "hashmap.h"

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

int main(void) {
    Ext_Arena a = ext_new_arena(NULL, 0, 0, 0);
    Ext_Context ctx = *ext_context;
    ctx.alloc = &a.base;
    ext_push_context(&ctx);

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
    ext_arena_reset(&a);

    IntArray arr = {0};
    for(int i = 0; i < 200; i++) {
        array_push(&arr, i);
    }

    array_foreach(int, elem, &arr) {
        printf("%d ", *elem);
    }
    printf("\n");

    ext_pop_context();
    ext_arena_free(&a);

    return 0;
}
