#include <stdio.h>

#include "hashmap.h"

typedef struct {
    int key;
    int value;
} IntEntry;

typedef struct {
    int key;
    size_t value;
} TestEntry;

typedef struct {
    HMAP_ENTRIES(IntEntry);
    size_t size;
    size_t numentries;
    size_t capacity;
} IntHashMap;

int main(void) {
    IntHashMap map = {0};
    for(int i = 0; i < 22; i++) {
        IntEntry e = {.key = i, .value = i * 10};
        hmap_put(&map, &e);
    }

    // printf("number of entries: %zu\n", map.size);
    // hmap_put(&map, &((IntEntry){.key = 2, .value = 100}));
    // hmap_delete(&map, &((IntEntry){.key = 2}));

    // IntEntry *entry;
    // hmap_get(&map, &((IntEntry){.key = 2}), &entry);
    // assert(entry == NULL);

    // hmap_get(&map, &((IntEntry){.key = 100}), &entry);
    // assert(entry == NULL);

    // hmap_get(&map, &((IntEntry){.key = 3}), &entry);
    // assert(entry != NULL);
    // printf("Key: %d, Value: %d\n", entry->key, entry->value);
    // printf("number of entries: %zu\n", map.size);

    for(const IntEntry* e = hmap_begin(&map); e != hmap_end(&map); e = hmap_next(&map, e)) {
        printf("Key: %d, Value: %d\n", e->key, e->value);
    }

    hmap_free(&map);
    return 0;
}

// #include "array.h"
//
// typedef struct {
//     int *data;
//     size_t size;
//     size_t capacity;
// } IntArray;
//
// int main(void) {
//     IntArray arr = {0};
//     array_push(&arr, 1);
//     array_push(&arr, 2);
//     array_push(&arr, 3);
//     array_push(&arr, 4);
//
//     array_foreach(int *elem, &arr) {
//         printf("%d ", *elem);
//     }
//     printf("\n");
//
//     array_free(&arr);
//     return 0;
// }
