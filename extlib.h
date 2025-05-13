#ifndef EXTLIB_H
#define EXTLIB_H

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef EXTLIB_NO_THREADSAFE
    #if defined(_MSC_VER)
        // MSVC supports __declspec(thread), but not for dynamically initialized data
        #define EXT_TLS __declspec(thread)
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && \
        !defined(__STDC_NO_THREADS__)
        #include <threads.h>
        #define EXT_TLS thread_local
    #elif defined(__GNUC__) || defined(__clang__)
        // GCC and Clang (even with C99 or C89)
        #define EXT_TLS __thread
    #else
        #warning \
            "thread local is not supported on this compiler. Fallback to global (non-thread-safe) storage."
        #define EXT_TLS
    #endif
#else
    #define EXT_TLS
#endif

// -----------------------------------------------------------------------------
// Allocator API
//

#define ext_new(T)         ext_alloc(sizeof(T))
#define ext_delete(T, ptr) ext_free(ptr, sizeof(T))

#define ext_alloc(size) ext_context->alloc->alloc(ext_context->alloc, size)
#define ext_realloc(ptr, old_size, new_size) \
    ext_context->alloc->realloc(ext_context->alloc, ptr, old_size, new_size)
#define ext_free(ptr, size) ext_context->alloc->free(ext_context->alloc, ptr, size)

typedef struct Ext_Allocator {
    void *(*alloc)(struct Ext_Allocator *, size_t size);
    void *(*realloc)(struct Ext_Allocator *, void *ptr, size_t old_size, size_t new_size);
    void (*free)(struct Ext_Allocator *, void *ptr, size_t size);
} Ext_Allocator;

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_Allocator Allocator;
#endif

// -----------------------------------------------------------------------------
// Default allocator
//

typedef struct Ext_DefaultAllocator {
    Ext_Allocator base;
} Ext_DefaultAllocator;
extern Ext_DefaultAllocator ext_default_allocator;

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_DefaultAllocator DefaultAllocator;
#endif

// -----------------------------------------------------------------------------
// Temp allocator
//

typedef struct Ext_TempAllocator {
    Ext_Allocator base;
    char *start, *end;
    size_t mem_size;
    void *mem;
} Ext_TempAllocator;
extern EXT_TLS Ext_TempAllocator ext_temp_allocator;

void ext_temp_set_mem(void *mem, size_t size);
void *ext_temp_alloc(size_t size);
void *ext_temp_realloc(void *ptr, size_t old_size, size_t new_size);
size_t ext_temp_available(void);
void ext_temp_reset(void);
void *ext_temp_checkpoint(void);
void ext_temp_rewind(void *checkpoint);
char *ext_temp_strdup(const char *str);
char *ext_temp_sprintf(const char *fmt, ...);
char *ext_temp_vsprintf(const char *fmt, va_list ap);

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_TempAllocator TempAllocator;

    #define temp_set_mem    ext_temp_set_mem
    #define temp_alloc      ext_temp_alloc
    #define temp_realloc    ext_temp_realloc
    #define temp_available  ext_temp_available
    #define temp_reset      ext_temp_reset
    #define temp_checkpoint ext_temp_checkpoint
    #define temp_rewind     ext_temp_rewind
    #define temp_strdup     ext_temp_strdup
    #define temp_sprintf    ext_temp_sprintf
    #define temp_vsprintf   ext_temp_vsprintf
#endif

#ifdef EXTLIB_IMPL

    #include <errno.h>
    #include <stdio.h>
    #include <string.h>

static void *ext_default_alloc(Ext_Allocator *a, size_t size) {
    (void)a;
    void *mem = malloc(size);
    if(!mem) {
        fprintf(stderr, "%s:%d: memory allocation failed: %s\n", __FILE__, __LINE__,
                strerror(errno));
        abort();
    }
    return mem;
}

static void *ext_default_realloc(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    (void)a;
    (void)old_size;
    void *mem = realloc(ptr, new_size);
    if(!mem) {
        fprintf(stderr, "%s:%d: memory allocation failed: %s\n", __FILE__, __LINE__,
                strerror(errno));
        abort();
    }
    return mem;
}

static void ext_default_free(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)size;
    free(ptr);
}

// -----------------------------------------------------------------------------
// Default allocator
//

Ext_DefaultAllocator ext_default_allocator = {
    {.alloc = ext_default_alloc, .realloc = ext_default_realloc, .free = ext_default_free},
};

// -----------------------------------------------------------------------------
// Global temp allocator
//

static void *ext_temp_alloc_wrap(Ext_Allocator *a, size_t size);
static void *ext_temp_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size);
static void ext_temp_free_wrap(Ext_Allocator *a, void *ptr, size_t size);

    #define EXT_DEFAULT_ALIGN 16
    #ifndef EXT_DEAFULT_TEMP_SIZE
        #define EXT_DEAFULT_TEMP_SIZE (256 * 1024 * 1024)
    #endif

static char temp_mem[EXT_DEAFULT_TEMP_SIZE];
EXT_TLS Ext_TempAllocator ext_temp_allocator = {
    {
        .alloc = ext_temp_alloc_wrap,
        .realloc = ext_temp_realloc_wrap,
        .free = ext_temp_free_wrap,
    },
    .start = temp_mem,
    .end = temp_mem + EXT_DEAFULT_TEMP_SIZE,
    .mem_size = EXT_DEAFULT_TEMP_SIZE,
    .mem = temp_mem,
};

static void *ext_temp_alloc_wrap(Ext_Allocator *a, size_t size) {
    (void)a;
    return ext_temp_alloc(size);
}

static void *ext_temp_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    (void)a;
    return ext_temp_realloc(ptr, old_size, new_size);
}

static void ext_temp_free_wrap(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)ptr;
    (void)size;
    // No-op, temp allocator does not free memory
}

void ext_temp_set_mem(void *mem, size_t size) {
    ext_temp_allocator.mem_size = size;
    ext_temp_allocator.mem = mem;
    ext_temp_reset();
}

void *ext_temp_alloc(size_t size) {
    ptrdiff_t alignment = -size & (EXT_DEFAULT_ALIGN - 1);
    ptrdiff_t available = ext_temp_allocator.end - ext_temp_allocator.start - alignment;
    if((ptrdiff_t)size > available) {
        fprintf(stderr, "%s:%d: temp allocation failed: %zu bytes requested, %zu bytes available\n",
                __FILE__, __LINE__, size, available);
        abort();
    }
    void *p = ext_temp_allocator.start;
    ext_temp_allocator.start += size + alignment;
    return p;
}

void *ext_temp_realloc(void *ptr, size_t old_size, size_t new_size) {
    ptrdiff_t alignment = -(uintptr_t)old_size & (EXT_DEFAULT_ALIGN - 1);
    // Reallocating last allocated memory, can grow/shrink in-place
    if(ext_temp_allocator.start - old_size - alignment == ptr) {
        ext_temp_allocator.start -= old_size + alignment;
        return ext_temp_alloc(new_size);
    } else if(new_size > old_size) {
        void *new_ptr = ext_temp_alloc(new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    } else {
        return ptr;
    }
}

size_t ext_temp_available(void) {
    return ext_temp_allocator.end - ext_temp_allocator.start;
}

void ext_temp_reset(void) {
    ext_temp_allocator.start = ext_temp_allocator.mem;
    ext_temp_allocator.end = (char *)ext_temp_allocator.mem + ext_temp_allocator.mem_size;
}

void *ext_temp_checkpoint(void) {
    return ext_temp_allocator.start;
}

void ext_temp_rewind(void *checkpoint) {
    ext_temp_allocator.start = checkpoint;
}

char *ext_temp_strdup(const char *str) {
    size_t n = strlen(str);
    char *res = ext_temp_alloc(n + 1);
    memcpy(res, str, n);
    res[n] = '\0';
    return res;
}

char *ext_temp_sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *res = ext_temp_vsprintf(fmt, ap);
    va_end(ap);
    return res;
}

char *ext_temp_vsprintf(const char *fmt, va_list ap) {
    va_list cpy;
    va_copy(cpy, ap);
    int n = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);

    assert(n >= 0 && "error in vsnprintf");
    char *res = ext_temp_alloc(n + 1);

    va_copy(cpy, ap);
    vsnprintf(res, n + 1, fmt, cpy);
    va_end(cpy);

    return res;
}
#endif  // EXTLIB_IMPL

// -----------------------------------------------------------------------------
// Context
//

#define EXT_PUSH_CONTEXT(ctx, code) \
    do {                            \
        push_context(ctx);          \
        code;                       \
        pop_context();              \
    } while(0)

typedef struct Ext_Context {
    struct Ext_Allocator *alloc;
    struct Ext_Context *prev;
} Ext_Context;
extern EXT_TLS Ext_Context *ext_context;

void ext_push_context(Ext_Context *ctx);
Ext_Context *ext_pop_context();

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_Context Context;
    #define PUSH_CONTEXT     EXT_PUSH_CONTEXT
    #define ext_push_context push_context
    #define ext_pop_context  pop_context
#endif

#ifdef EXTLIB_IMPL
EXT_TLS Ext_Context *ext_context = &(Ext_Context){
    .alloc = (Ext_Allocator *)&ext_default_allocator,
};

void ext_push_context(Ext_Context *ctx) {
    ctx->prev = ext_context;
    ext_context = ctx;
}

Ext_Context *ext_pop_context() {
    assert(ext_context->prev && "Trying to pop default allocator");
    Ext_Context *old_ctx = ext_context;
    ext_context = old_ctx->prev;
    return old_ctx;
}
#endif

// -----------------------------------------------------------------------------
// Arena allocator
//

typedef enum {
    EXT_ARENA_NONE = 0,
    // Forces the arena to behave like a stack allocator. The arena will
    // check that frees are done only in LIFO order. If this is not the
    // case, it will abort with an error.
    EXT_ARENA_STACK_ALLOC = 1 << 0,
    // Zeroes the memory allocated by the arena.
    EXT_ARENA_ZERO_ALLOC = 1 << 1,
    // When a single allocation requests more than `page_size` bytes, the arena will request a
    // larger page from the system instead of failing.
    EXT_ARENA_FLEXIBLE_PAGE = 1 << 2,
} Ext_ArenaFlags;

typedef struct Ext_ArenaPage {
    struct Ext_ArenaPage *next;
    char *start, *end;
    char data[];
} Ext_ArenaPage;

// TODO: implement maximum size
typedef struct Ext_Arena {
    Ext_Allocator base;
    const size_t alignment;
    const size_t page_size;
    Ext_Allocator *const page_allocator;
    const Ext_ArenaFlags flags;
    Ext_ArenaPage *first_page, *last_page;
    size_t allocated;
} Ext_Arena;

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags);
void *ext_arena_alloc(Ext_Arena *a, size_t size);
void *ext_arena_realloc(Ext_Arena *a, void *ptr, size_t old_size, size_t new_size);
void ext_arena_dealloc(Ext_Arena *a, void *ptr, size_t size);
void ext_arena_reset(Ext_Arena *a);
void ext_arena_free(Ext_Arena *a);
char *ext_arena_strdup(Ext_Arena *a, const char *str);
char *ext_arena_sprintf(Ext_Arena *a, const char *fmt, ...);
char *ext_arena_vsprintf(Ext_Arena *a, const char *fmt, va_list ap);

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_ArenaFlags ArenaFlags;
typedef Ext_Arena Arena;
typedef Ext_ArenaPage ArenaPage;
    #define new_arena      ext_new_arena
    #define arena_alloc    ext_arena_alloc
    #define arena_realloc  ext_arena_realloc
    #define arena_dealloc  ext_arena_dealloc
    #define arena_reset    ext_arena_reset
    #define arena_free     ext_arena_free
    #define arena_strdup   ext_arena_strdup
    #define arena_sprintf  ext_arena_sprintf
    #define arena_vsprintf ext_arena_vsprintf
#endif  // EXTLIB_NO_SHORTHANDS

#ifdef EXTLIB_IMPL
    #define EXT_ARENA_PAGE_SZ       (4 * 1024)  // 4 KiB
    #define EXT_EXT_ARENA_ALIGNMENT (16)
    #define EXT_ARENA_ALIGN(o, a)   (-(uintptr_t)(o) & ((a)->alignment - 1))

static Ext_ArenaPage *ext_arena_new_page(Ext_Arena *arena, size_t requested_size) {
    size_t header_alignment = EXT_ARENA_ALIGN(sizeof(Ext_ArenaPage), arena);
    size_t page_size = arena->page_size;
    if(requested_size + header_alignment > page_size) {
        if(arena->flags & EXT_ARENA_FLEXIBLE_PAGE) {
            page_size = requested_size + header_alignment;
        } else {
            fprintf(stderr,
                    "Error: requested size %zu exceeds max allocatable size in page (%zu)\n",
                    requested_size, arena->page_size - (sizeof(Ext_ArenaPage) + header_alignment));
            abort();
        }
    }

    Ext_ArenaPage *page = arena->page_allocator->alloc(arena->page_allocator, page_size);
    page->next = NULL;

    // Account for alignment of first allocation; the arena assumes every pointer starts aligned to
    // the arena's alignment.
    page->start = page->data + EXT_ARENA_ALIGN(page->data, arena);
    page->end = page->data + page_size;
    return page;
}

static void *ext_arena_alloc_wrap(Ext_Allocator *a, size_t size) {
    return ext_arena_alloc((Ext_Arena *)a, size);
}

static void *ext_arena_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    return ext_arena_realloc((Ext_Arena *)a, ptr, old_size, new_size);
}

static void ext_arena_free_wrap(Ext_Allocator *a, void *ptr, size_t size) {
    ext_arena_dealloc((Ext_Arena *)a, ptr, size);
}

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags) {
    if(!alignment) alignment = EXT_EXT_ARENA_ALIGNMENT;
    if(!page_size) page_size = EXT_ARENA_PAGE_SZ;
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");
    assert(page_size > sizeof(Ext_ArenaPage) + alignment &&
           "Page size must be greater the size of Ext_ArenaPage + alignment bytes");
    return (Ext_Arena){
        .base =
            {
                .alloc = ext_arena_alloc_wrap,
                .realloc = ext_arena_realloc_wrap,
                .free = ext_arena_free_wrap,
            },
        .alignment = alignment,
        .page_size = page_size,
        .page_allocator = page_alloc ? page_alloc : ext_context->alloc,
        .flags = flags,
    };
}

void *ext_arena_alloc(Ext_Arena *a, size_t size) {
    Ext_Arena *arena = (Ext_Arena *)a;

    if(!arena->last_page) {
        assert(arena->first_page == NULL && arena->allocated == 0);
        Ext_ArenaPage *page = ext_arena_new_page(arena, size);
        arena->first_page = page;
        arena->last_page = page;
    }

    size_t alignment = EXT_ARENA_ALIGN(size, arena);
    ptrdiff_t available = arena->last_page->end - arena->last_page->start - alignment;
    while(available < (ptrdiff_t)size) {
        Ext_ArenaPage *next_page = arena->last_page->next;
        if(!next_page) {
            arena->last_page->next = ext_arena_new_page(arena, size);
            arena->last_page = arena->last_page->next;
            available = arena->last_page->end - arena->last_page->start - alignment;
            break;
        } else {
            arena->last_page = next_page;
            available = next_page->end - next_page->start - alignment;
        }
    }
    assert(available >= (ptrdiff_t)size && "Not enough space in arena");

    void *p = arena->last_page->start;
    assert(EXT_ARENA_ALIGN(p, arena) == 0 && "Pointer is not aligned to the arena's alignment");
    arena->last_page->start += size + alignment;
    arena->allocated += size + alignment;

    if(arena->flags & EXT_ARENA_ZERO_ALLOC) {
        memset(p, 0, size);
    }

    return p;
}

void *ext_arena_realloc(Ext_Arena *a, void *ptr, size_t old_size, size_t new_size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    assert(EXT_ARENA_ALIGN(old_size, arena) == 0 &&
           "Old size is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    assert(page && "No pages in arena");

    size_t alignment = EXT_ARENA_ALIGN(ptr, arena);
    if(page->start - old_size - alignment == ptr) {
        // Reallocating last allocated memory, can grow/shrink in-place
        page->start -= old_size + alignment;
        arena->allocated -= old_size + alignment;
        return ext_arena_alloc(a, new_size);
    } else if(new_size > old_size) {
        void *new_ptr = ext_arena_alloc(a, new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    } else {
        return ptr;
    }
}

void ext_arena_dealloc(Ext_Arena *a, void *ptr, size_t size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    assert(EXT_ARENA_ALIGN(size, arena) == 0 && "Size is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    assert(page && "No pages in arena");

    size_t alignment = EXT_ARENA_ALIGN(size, arena);
    if(page->start - size - alignment == ptr) {
        // Deallocating last allocated memory, can shrink in-place
        page->start -= size + alignment;
        arena->allocated -= size + alignment;
        return;
    }

    // In stack allocator mode force LIFO order
    if(arena->flags & EXT_ARENA_STACK_ALLOC) {
        fprintf(stderr, "Deallocating memory in non-LIFO order: got %p, expected %p\n", ptr,
                (void *)(page->start - size - alignment));
        abort();
    }
}

void ext_arena_reset(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        page->start = page->data + EXT_ARENA_ALIGN(page->data, a);
        page->end = page->data + a->page_size;
        page = page->next;
    }
    a->last_page = a->first_page;
    a->allocated = 0;
}

void ext_arena_free(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        Ext_ArenaPage *next = page->next;
        a->page_allocator->free(a->page_allocator, page, a->page_size);
        page = next;
    }
    a->first_page = NULL;
    a->last_page = NULL;
    a->allocated = 0;
}

char *ext_arena_strdup(Ext_Arena *a, const char *str) {
    size_t n = strlen(str);
    char *res = ext_arena_alloc(a, n + 1);
    memcpy(res, str, n);
    res[n] = '\0';
    return res;
}

char *ext_arena_sprintf(Ext_Arena *a, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *res = ext_arena_vsprintf(a, fmt, ap);
    va_end(ap);
    return res;
}

char *ext_arena_vsprintf(Ext_Arena *a, const char *fmt, va_list ap) {
    va_list cpy;
    va_copy(cpy, ap);
    int n = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);

    assert(n >= 0 && "error in vsnprintf");
    char *res = ext_arena_alloc(a, n + 1);

    va_copy(cpy, ap);
    vsnprintf(res, n + 1, fmt, cpy);
    va_end(cpy);

    return res;
}
#endif  // EXTLIB_IMPL

// -----------------------------------------------------------------------------
// Dynamic array
//

#ifndef EXT_ARRAY_INIT_CAP
    #define EXT_ARRAY_INIT_CAP 8
#endif  // EXT_ARRAY_INIT_CAP

#define EXT_ARRAY_ITEMS(T) \
    T *items;              \
    size_t size, capacity; \
    Ext_Allocator *allocator;

#define ext_array_foreach(T, it, vec) \
    for(T *it = (vec)->items, *end = (vec)->items + (vec)->size; it != end; it++)

#define ext_array_reserve(arr, newcap)                                                     \
    do {                                                                                   \
        if((arr)->capacity < (newcap)) {                                                   \
            size_t oldcap = (arr)->capacity;                                               \
            (arr)->capacity = oldcap ? (arr)->capacity * 2 : EXT_ARRAY_INIT_CAP;           \
            while((arr)->capacity < (newcap)) {                                            \
                (arr)->capacity <<= 1;                                                     \
            }                                                                              \
            if(!((arr)->allocator)) {                                                      \
                (arr)->allocator = ext_context->alloc;                                     \
            }                                                                              \
            Ext_Allocator *a = (arr)->allocator;                                           \
            if(!(arr)->items) {                                                            \
                (arr)->items = a->alloc(a, (arr)->capacity * sizeof(*(arr)->items));       \
            } else {                                                                       \
                (arr)->items = a->realloc(a, (arr)->items, oldcap * sizeof(*(arr)->items), \
                                          (arr)->capacity * sizeof(*(arr)->items));        \
            }                                                                              \
        }                                                                                  \
    } while(0)

#define ext_array_push(a, v)                   \
    do {                                       \
        ext_array_reserve((a), (a)->size + 1); \
        (a)->items[(a)->size++] = (v);         \
    } while(0)

#define ext_array_free(a)                                                                          \
    do {                                                                                           \
        if((a)->allocator) {                                                                       \
            (a)->allocator->free((a)->allocator, (a)->items, (a)->capacity * sizeof(*(a)->items)); \
        }                                                                                          \
        memset((a), 0, sizeof(*(a)));                                                              \
    } while(0)

#ifndef EXTLIB_NO_SHORTHANDS
    #define ARRAY_ITEMS   EXT_ARRAY_ITEMS
    #define array_foreach ext_array_foreach
    #define array_reserve ext_array_reserve
    #define array_push    ext_array_push
    #define array_free    ext_array_free
#endif  // EXT_NO_SHORTHANDS

// -----------------------------------------------------------------------------
// Hashmap
//

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
            (hmap)->allocator = ext_context->alloc;                                         \
        }                                                                                   \
        Ext_Allocator *a = (hmap)->allocator;                                               \
        void *newentries = a->alloc(a, totalsz);                                            \
        size_t *newhashes = (size_t *)((char *)newentries + newsz + pad);                   \
        memset(newhashes, 0, sizeof(*(hmap)->hashes) * newcap);                             \
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
#endif  // EXTLIB_H
