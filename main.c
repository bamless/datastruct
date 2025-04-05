#include <stdint.h>
#include <stdio.h>

#include "array.h"
#include "hashmap.h"

typedef struct {
    int key;
    int value;
} IntEntry;

typedef struct {
    IntEntry *entries;
    size_t *hashes;
    size_t size, capacity;
} IntHashMap;

typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} IntArray;

int main(void) {
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

    hmap_foreach(IntEntry, e, &map) {
        printf("Key: %d, Value: %d\n", e->key, e->value);
    }

    hmap_free(&map);

    printf("Array ----------------------------\n");

    IntArray arr = {0};
    array_push(&arr, 1);
    array_push(&arr, 2);
    array_push(&arr, 3);
    array_push(&arr, 4);

    array_foreach(int, elem, &arr) {
        printf("%d ", *elem);
    }
    printf("\n");

    array_free(&arr);
    return 0;
}
