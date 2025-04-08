#ifndef HASHMAP_H
#define HASHMAP_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"

// TODO: require allocator in struct!!

// Read as: size * 0.75, i.e. a load factor of 75%
// This is basically doing:
//   size / 2 + size / 4 = (3 * size) / 4
#define HMAP_MAX_ENTRY_LOAD(size) (((size) >> 1) + ((size) >> 2))
#define HMAP_EMPTY_MARK           0
#define HMAP_TOMB_MARK            1
#ifndef HMAP_INIT_CAPACITY
    #define HMAP_INIT_CAPACITY 8
#endif
// TODO: static assert power of 2 for capacity
#define HMAP_IS_TOMB(h)  ((h) == HMAP_TOMB_MARK)
#define HMAP_IS_EMPTY(h) ((h) == HMAP_EMPTY_MARK)
#define HMAP_IS_VALID(h) (!HMAP_IS_EMPTY(h) && !HMAP_IS_TOMB(h))

#define EXT_HMAP_PADDING(a, o) ((a - (o & (a - 1))) & (a - 1))

#define EXT_HMAP_ENTRIES(T) \
    T *entries;             \
    size_t *hashes;         \
    size_t size, capacity;  \
    Ext_Allocator *allocator;

#define hmap_foreach(T, it, hmap) \
    for(T *it = hmap_begin(hmap), *end = hmap_end(hmap); it != end; it = hmap_next(hmap, it))

#define hmap_end(hmap) hmap_end__((hmap)->entries, (hmap)->capacity, sizeof(*(hmap)->entries))
#define hmap_begin(hmap) \
    hmap_begin__((hmap)->entries, (hmap)->hashes, (hmap)->capacity, sizeof(*(hmap)->entries))
#define hmap_next(hmap, it) \
    hmap_next__((hmap)->entries, (hmap)->hashes, it, (hmap)->capacity, sizeof(*(hmap)->entries))

#define hmap_put(hmap, entry)                                                   \
    do {                                                                        \
        if((hmap)->size >= HMAP_MAX_ENTRY_LOAD((hmap)->capacity + 1)) {         \
            hmap_grow__(hmap);                                                  \
        }                                                                       \
        size_t hash = stbds_hash_bytes(&(entry)->key, sizeof((entry)->key), 0); \
        if(hash < 2) hash += 2;                                                 \
        hmap_find_index__(hmap, entry, hash, hmap_memcmp__);                    \
        if(!HMAP_IS_VALID((hmap)->hashes[idx])) {                               \
            (hmap)->size++;                                                     \
        }                                                                       \
        (hmap)->hashes[idx] = hash;                                             \
        (hmap)->entries[idx] = *(entry);                                        \
    } while(0)

#define hmap_get(hmap, entry, out)                                              \
    do {                                                                        \
        *out = entry;                                                           \
        size_t hash = stbds_hash_bytes(&(entry)->key, sizeof((entry)->key), 0); \
        if(hash < 2) hash += 2;                                                 \
        hmap_find_index__(hmap, entry, hash, hmap_memcmp__);                    \
        if(HMAP_IS_VALID((hmap)->hashes[idx])) {                                \
            *(out) = &(hmap)->entries[idx];                                     \
        } else {                                                                \
            *(out) = NULL;                                                      \
        }                                                                       \
    } while(0)

#define hmap_delete(hmap, entry)                                                \
    do {                                                                        \
        size_t hash = stbds_hash_bytes(&(entry)->key, sizeof((entry)->key), 0); \
        if(hash < 2) hash += 2;                                                 \
        hmap_find_index__(hmap, entry, hash, hmap_memcmp__);                    \
        if(HMAP_IS_VALID((hmap)->hashes[idx])) {                                \
            (hmap)->hashes[idx] = HMAP_TOMB_MARK;                               \
            (hmap)->size--;                                                     \
        }                                                                       \
    } while(0)

#define hmap_clear(hmap)                                                             \
    do {                                                                             \
        memset((hmap)->hashes, 0, sizeof(*(hmap)->hashes) * ((hmap)->capacity + 1)); \
        (hmap)->size = 0;                                                            \
    } while(0)

#define hmap_free(hmap)                     \
    do {                                    \
        hmap_free__(hmap);                  \
        memset((hmap), 0, sizeof(*(hmap))); \
    } while(0)

#define hmap_memcmp__(a, b) memcmp((a), (b), sizeof(*(a)))

#define hmap_grow__(hmap)                                                                   \
    do {                                                                                    \
        size_t newcap = (hmap)->capacity ? ((hmap)->capacity + 1) * 2 : HMAP_INIT_CAPACITY; \
        size_t newsz = newcap * sizeof(*(hmap)->entries);                                   \
        size_t pad = EXT_HMAP_PADDING(sizeof(*(hmap)->hashes), newsz);                      \
        size_t totalsz = newsz + pad + sizeof(*(hmap)->hashes) * newcap;                    \
        if(!(hmap)->allocator) {                                                            \
            (hmap)->allocator = ext_allocator_ctx;                                          \
        }                                                                                   \
        Ext_Allocator *a = (hmap)->allocator;                                               \
        void *newentries = a->alloc(a, totalsz);                                            \
        size_t *newhashes = (size_t *)((char *)newentries + newsz + pad);                   \
        if((hmap)->capacity > 0) {                                                          \
            for(size_t i = 0; i <= (hmap)->capacity; i++) {                                 \
                size_t hash = (hmap)->hashes[i];                                            \
                if(HMAP_IS_VALID(hash)) {                                                   \
                    size_t newidx = hash & (newcap - 1);                                    \
                    while(!HMAP_IS_EMPTY(newhashes[newidx])) {                              \
                        newidx = (newidx + 1) & (newcap - 1);                               \
                    }                                                                       \
                    memcpy((char *)newentries + newidx * sizeof(*(hmap)->entries),          \
                           (hmap)->entries + i, sizeof(*(hmap)->entries));                  \
                    newhashes[newidx] = hash;                                               \
                }                                                                           \
            }                                                                               \
        }                                                                                   \
        hmap_free__(hmap);                                                                  \
        (hmap)->entries = newentries;                                                       \
        (hmap)->hashes = newhashes;                                                         \
        (hmap)->capacity = newcap - 1;                                                      \
    } while(0)

#define hmap_find_index__(map, entry, hash, cmp)                                         \
    size_t idx = 0;                                                                      \
    {                                                                                    \
        size_t i = (hash) & (map)->capacity;                                             \
        bool tomb_found = false;                                                         \
        size_t tomb_idx = 0;                                                             \
        for(;;) {                                                                        \
            size_t buck = (map)->hashes[i];                                              \
            if(!HMAP_IS_VALID(buck)) {                                                   \
                if(HMAP_IS_EMPTY(buck)) {                                                \
                    idx = tomb_found ? tomb_idx : i;                                     \
                    break;                                                               \
                } else if(!tomb_found) {                                                 \
                    tomb_found = true;                                                   \
                    tomb_idx = i;                                                        \
                }                                                                        \
            } else if(buck == hash && cmp(&(entry)->key, &(map)->entries[i].key) == 0) { \
                idx = i;                                                                 \
                break;                                                                   \
            }                                                                            \
            i = (i + 1) & (map)->capacity;                                               \
        }                                                                                \
    }

#define hmap_free__(hmap)                                                                 \
    do {                                                                                  \
        if((hmap)->allocator) {                                                           \
            size_t sz = ((hmap)->capacity + 1) * sizeof(*(hmap)->entries);                \
            size_t pad = EXT_HMAP_PADDING(sizeof(*(hmap)->hashes), sz);                   \
            size_t totalsz = sz + pad + sizeof(*(hmap)->hashes) * ((hmap)->capacity + 1); \
            (hmap)->allocator->free((hmap)->allocator, (hmap)->entries, totalsz);         \
        }                                                                                 \
    } while(0)

#ifndef EXTLIB_NO_SHORTHANDS
    #define HMAP_ENTRIES EXT_HMAP_ENTRIES
#endif

static inline void *hmap_end__(const void *entries, size_t cap, size_t sz) {
    return entries ? (char *)entries + (cap + 1) * sz : NULL;
}

static inline void *hmap_begin__(const void *entries, const size_t *hashes, size_t cap, size_t sz) {
    if(!entries) return NULL;
    for(size_t i = 0; i <= cap; i++) {
        if(HMAP_IS_VALID(hashes[i])) {
            return (char *)entries + i * sz;
        }
    }
    return hmap_end__(entries, cap, sz);
}

static inline void *hmap_next__(const void *entries, const size_t *hashes, const void *it,
                                size_t cap, size_t sz) {
    size_t curr = ((char *)it - (char *)entries) / sz;
    for(size_t idx = curr + 1; idx <= cap; idx++) {
        if(HMAP_IS_VALID(hashes[idx])) {
            return (char *)entries + idx * sz;
        }
    }
    return hmap_end__(entries, cap, sz);
}

// ============================================================================
// Taken from stb_ds.h
// TODO: add stb_ds license
// TODO: put under EXTLIB_IMPLEMENTATION flag

#define EXT_SIZET_BITS           ((sizeof(size_t)) * CHAR_BIT)
#define EXT_ROTATE_LEFT(val, n)  (((val) << (n)) | ((val) >> (EXT_SIZET_BITS - (n))))
#define EXT_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (EXT_SIZET_BITS - (n))))

static inline size_t stbds_hash_string(char *str, size_t seed) {
    size_t hash = seed;
    while(*str) hash = EXT_ROTATE_LEFT(hash, 9) + (unsigned char)*str++;

    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ EXT_ROTATE_RIGHT(hash, 31);
    hash = hash * 21;
    hash ^= hash ^ EXT_ROTATE_RIGHT(hash, 11);
    hash += (hash << 6);
    hash ^= EXT_ROTATE_RIGHT(hash, 22);
    return hash + seed;
}

#ifdef STBDS_SIPHASH_2_4
    #define STBDS_SIPHASH_C_ROUNDS 2
    #define STBDS_SIPHASH_D_ROUNDS 4
typedef int STBDS_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(size_t) == 8 ? 1 : -1];
#endif

#ifndef STBDS_SIPHASH_C_ROUNDS
    #define STBDS_SIPHASH_C_ROUNDS 1
#endif
#ifndef STBDS_SIPHASH_D_ROUNDS
    #define STBDS_SIPHASH_D_ROUNDS 1
#endif

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning( \
        disable : 4127)  // conditional expression is constant, for do..while(0) and sizeof()==
#endif

static size_t stbds_siphash_bytes(void *p, size_t len, size_t seed) {
    unsigned char *d = (unsigned char *)p;
    size_t i, j;
    size_t v0, v1, v2, v3, data;

    // hash that works on 32- or 64-bit registers without knowing which we have
    // (computes different results on 32-bit and 64-bit platform)
    // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4
    // 64-bit
    v0 = ((((size_t)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
    v1 = ((((size_t)0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
    v2 = ((((size_t)0x6c796765 << 16) << 16) + 0x6e657261) ^ seed;
    v3 = ((((size_t)0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

#ifdef STBDS_TEST_SIPHASH_2_4
    // hardcoded with key material in the siphash test vectors
    v0 ^= 0x0706050403020100ull ^ seed;
    v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
    v2 ^= 0x0706050403020100ull ^ seed;
    v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
#endif

#define STBDS_SIPROUND()                              \
    do {                                              \
        v0 += v1;                                     \
        v1 = EXT_ROTATE_LEFT(v1, 13);                 \
        v1 ^= v0;                                     \
        v0 = EXT_ROTATE_LEFT(v0, EXT_SIZET_BITS / 2); \
        v2 += v3;                                     \
        v3 = EXT_ROTATE_LEFT(v3, 16);                 \
        v3 ^= v2;                                     \
        v2 += v1;                                     \
        v1 = EXT_ROTATE_LEFT(v1, 17);                 \
        v1 ^= v2;                                     \
        v2 = EXT_ROTATE_LEFT(v2, EXT_SIZET_BITS / 2); \
        v0 += v3;                                     \
        v3 = EXT_ROTATE_LEFT(v3, 21);                 \
        v3 ^= v0;                                     \
    } while(0)

    for(i = 0; i + sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
        data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        data |= (size_t)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24))
                << 16 << 16;  // discarded if size_t == 4

        v3 ^= data;
        for(j = 0; j < STBDS_SIPHASH_C_ROUNDS; ++j) STBDS_SIPROUND();
        v0 ^= data;
    }
    data = len << (EXT_SIZET_BITS - 8);
    switch(len - i) {
    case 7:
        data |= ((size_t)d[6] << 24) << 24;  // fall through
    case 6:
        data |= ((size_t)d[5] << 20) << 20;  // fall through
    case 5:
        data |= ((size_t)d[4] << 16) << 16;  // fall through
    case 4:
        data |= (d[3] << 24);  // fall through
    case 3:
        data |= (d[2] << 16);  // fall through
    case 2:
        data |= (d[1] << 8);  // fall through
    case 1:
        data |= d[0];  // fall through
    case 0:
        break;
    }
    v3 ^= data;
    for(j = 0; j < STBDS_SIPHASH_C_ROUNDS; ++j) STBDS_SIPROUND();
    v0 ^= data;
    v2 ^= 0xff;
    for(j = 0; j < STBDS_SIPHASH_D_ROUNDS; ++j) STBDS_SIPROUND();

#ifdef STBDS_SIPHASH_2_4
    return v0 ^ v1 ^ v2 ^ v3;
#else
    return v1 ^ v2 ^
           v3;  // slightly stronger since v0^v3 in above cancels out final round operation? I
                // tweeted at the authors of SipHash about this but they didn't reply
#endif
}

static inline size_t stbds_hash_bytes(void *p, size_t len, size_t seed) {
#ifdef STBDS_SIPHASH_2_4
    return stbds_siphash_bytes(p, len, seed);
#else
    unsigned char *d = (unsigned char *)p;

    if(len == 4) {
        unsigned int hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates
        // turned into shifts. Note that converting these back to rotates makes it run a lot slower,
        // presumably due to collisions, so I'm not really sure what's going on.
        hash ^= seed;
        hash = (hash ^ 61) ^ (hash >> 16);
        hash = hash + (hash << 3);
        hash = hash ^ (hash >> 4);
        hash = hash * 0x27d4eb2d;
        hash ^= seed;
        hash = hash ^ (hash >> 15);
        return (((size_t)hash << 16 << 16) | hash) ^ seed;
    } else if(len == 8 && sizeof(size_t) == 8) {
        size_t hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        hash |= (size_t)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24))
                << 16 << 16;  // avoid warning if size_t == 4
        hash ^= seed;
        hash = (~hash) + (hash << 21);
        hash ^= EXT_ROTATE_RIGHT(hash, 24);
        hash *= 265;
        hash ^= EXT_ROTATE_RIGHT(hash, 14);
        hash ^= seed;
        hash *= 21;
        hash ^= EXT_ROTATE_RIGHT(hash, 28);
        hash += (hash << 31);
        hash = (~hash) + (hash << 18);
        return hash;
    } else {
        return stbds_siphash_bytes(p, len, seed);
    }
#endif
}
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

static int key_equals(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset,
                      int mode, size_t i) {
    // if(mode >= STBDS_HM_STRING)
    //     return 0 == strcmp((char *)key, *(char **)((char *)a + elemsize * i + keyoffset));
    // else rreturn 0 == memcmp(key, (char *)a + elemsize * i + keyoffset, keysize);
    return 0 == memcmp(key, (char *)a + elemsize * i + keyoffset, keysize);
}

// End of stbds.h
// =============================================================================

#endif
