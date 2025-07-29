#ifndef EXTLIB_H
#define EXTLIB_H

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef EXTLIB_WASM
#ifndef EXTLIB_NO_STD
#define EXTLIB_NO_STD
#endif  // EXTLIB_NO_STD

#ifdef EXTLIB_THREADSAFE
#undef EXTLIB_THREADSAFE
#endif  // EXTLIB_THREADSAFE
#endif  // EXTLIB_WASM

#ifndef EXTLIB_NO_STD
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
static inline int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for(size_t i = 0; i < n; i++) {
        if(p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}
static inline void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for(size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}
static inline void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}
static inline size_t strlen(const char *s) {
    size_t len = 0;
    while(s[len] != '\0') len++;
    return len;
}
void assert(int c);
#endif  // EXTLIB_NO_STD

#ifdef EXTLIB_THREADSAFE
#if defined(_MSC_VER)
// MSVC supports __declspec(thread), but not for dynamically initialized data
#define EXT_TLS __declspec(thread)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_THREADS__)
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
#endif  // EXTLIB_THREADSAFE

#if defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
#define EXT_WINDOWS
#elif defined(__linux__)
#define EXT_LINUX
#define EXT_POSIX
#elif defined(__ANDROID__)
#define EXT_ANDROID
#define EXT_POSIX
#elif defined(__FreeBSD__)
#define EXT_FREEBSD
#define EXT_POSIX
#elif defined(__OpenBSD__)
#define EXT_OPENBSD
#define EXT_POSIX
#elif defined(__EMSCRIPTEN__)
#define EXT_EMSCRIPTEN
#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>

#if TARGET_OS_IPHONE == 1
#define EXT_IOS
#elif TARGET_OS_MAC == 1
#define EXT_MACOS
#endif

#define EXT_POSIX
#endif

// -----------------------------------------------------------------------------
// SECTION: macros
//
#ifndef NDEBUG

#ifndef EXTLIB_NO_STD
#define EXT_ASSERT(cond, msg)                                                               \
    ((cond) ? ((void)0)                                                                     \
            : (fprintf(stderr, "%s:%d: error in %s(): %s failed: %s\n", __FILE__, __LINE__, \
                       __func__, #cond, msg),                                               \
               abort()))

#define EXT_UNREACHABLE()                                                                     \
    (fprintf(stderr, "%s:%d: error in %s(): reached unreachable code.\n", __FILE__, __LINE__, \
             __func__),                                                                       \
     abort())
#else
#define EXT_ASSERT(cond, msg) assert((cond) && msg)
#define EXT_UNREACHABLE()     assert(false && "reached unreachable code")
#endif

#else
#define EXT_ASSERT(cond, msg) ((void)(cond))

#if defined(__GNUC__) || defined(__clang__)
#define EXT_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#include <stdlib.h>
#define EXT_UNREACHABLE() __assume(0)
#else
#define EXT_UNREACHABLE()
#endif  // defined(__GNUC__) || defined(__clang__)

#endif  // NDEBUG

#ifndef static_assert
#define EXT_CONCAT2_(pre, post) pre##post
#define EXT_CONCAT_(pre, post)  EXT_CONCAT2_(pre, post)
#define EXT_STATIC_ASSERT(cond, msg)            \
    typedef struct {                            \
        int static_assertion_failed : !!(cond); \
    } EXT_CONCAT_(static_assertion_failed_, __COUNTER__)
#else
#define EXT_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#endif

#ifndef EXT_DEFAULT_ALIGNMENT
#define EXT_DEFAULT_ALIGNMENT (16)
#endif  // EXT_DEFAULT_ALIGNMENT

EXT_STATIC_ASSERT(((EXT_DEFAULT_ALIGNMENT) & ((EXT_DEFAULT_ALIGNMENT)-1)) == 0,
                  "default alignment must be a power of 2");

#define EXT_ALIGN(o, s) (-(uintptr_t)(o) & (s - 1))
#define EXT_ARR_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifdef __GNUC__
#define EXT_PRINTF_FORMAT(a, b) __attribute__((format(printf, a, b)))
#else
#define EXT_PRINTF_FORMAT(a, b)
#endif  // __GNUC__

#ifndef EXT_NO_SHORTHANDS
#define ASSERT        EXT_ASSERT
#define UNREACHABLE   EXT_UNREACHABLE
#define STATIC_ASSERT EXT_STATIC_ASSERT
#define ALIGN         EXT_ALIGN
#define ARR_SIZE      EXT_ARR_SIZE
#define PRINTF_FORMAT EXT_PRINTF_FORMAT
#endif  // EXT_NO_SHORTHANDS

// -----------------------------------------------------------------------------
// SECTION: Allocator API
//
#define ext_new(T)                      ext_alloc(sizeof(T))
#define ext_new_array(T, count)         ext_alloc(sizeof(int) * count)
#define ext_delete(T, ptr)              ext_free(ptr, sizeof(T))
#define ext_delete_array(T, count, ptr) ext_free(ptr, sizeof(T) * count);

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
// SECTION: Default allocator
//
typedef struct Ext_DefaultAllocator {
    Ext_Allocator base;
} Ext_DefaultAllocator;
extern Ext_DefaultAllocator ext_default_allocator;

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_DefaultAllocator DefaultAllocator;
#endif

// -----------------------------------------------------------------------------
// SECTION: Temp allocator
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
void *ext_temp_memdup(void *mem, size_t size);
#ifndef EXTLIB_NO_STD
char *ext_temp_sprintf(const char *fmt, ...) EXT_PRINTF_FORMAT(1, 2);
char *ext_temp_vsprintf(const char *fmt, va_list ap);
#endif  // EXTLIB_NO_STD

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
#define temp_memdup     ext_temp_memdup
#ifndef EXTLIB_NO_STD
#define temp_sprintf  ext_temp_sprintf
#define temp_vsprintf ext_temp_vsprintf
#endif  // EXTLIB_NO_STD
#endif  // EXTLIB_NO_SHORTHANDS

#ifdef EXTLIB_IMPL

// -----------------------------------------------------------------------------
// SECTION: default allocator
//
#ifdef EXTLIB_WASM
extern char __heapbase[];
static void *ext_heap_start = (void *)__heapbase;
#endif  // EXTLIB_WASM

static void *ext_default_alloc(Ext_Allocator *a, size_t size) {
    (void)a;
#ifndef EXTLIB_NO_STD
    void *mem = malloc(size);
    EXT_ASSERT(mem, "out of memory");
    return mem;
#elif defined(EXTLIB_WASM)
    void *mem = ext_heap_start;
    ext_heap_start = (char *)ext_heap_start + size;
    return mem;
#else
    (void)size;
    return NULL;
#endif
}

static void *ext_default_realloc(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
#ifndef EXTLIB_NO_STD
    (void)a;
    (void)old_size;
    void *mem = realloc(ptr, new_size);
    EXT_ASSERT(mem, "out of memory");
    return mem;
#elif defined EXTLIB_WASM
    void *mem = ext_default_alloc(a, new_size);
    memcpy(mem, ptr, old_size);
    return mem;
#else
    (void)ptr;
    (void)new_size;
    return NULL
#endif
}

static void ext_default_free(Ext_Allocator *a, void *ptr, size_t size) {
    (void)a;
    (void)size;
#ifndef EXTLIB_NO_STD
    free(ptr);
#else
    (void)ptr;
#endif
}

Ext_DefaultAllocator ext_default_allocator = {
    {
        .alloc = ext_default_alloc,
        .realloc = ext_default_realloc,
        .free = ext_default_free,
    },
};

// -----------------------------------------------------------------------------
// SECTION: temp allocator
//
static void *ext_temp_alloc_wrap(Ext_Allocator *a, size_t size);
static void *ext_temp_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size);
static void ext_temp_free_wrap(Ext_Allocator *a, void *ptr, size_t size);

#ifndef EXT_DEAFULT_TEMP_SIZE
#define EXT_DEAFULT_TEMP_SIZE (8 * 1024 * 1024)
#endif

static char ext_temp_mem[EXT_DEAFULT_TEMP_SIZE];
EXT_TLS Ext_TempAllocator ext_temp_allocator = {
    {.alloc = ext_temp_alloc_wrap, .realloc = ext_temp_realloc_wrap, .free = ext_temp_free_wrap},
    .start = ext_temp_mem,
    .end = ext_temp_mem + EXT_DEAFULT_TEMP_SIZE,
    .mem_size = EXT_DEAFULT_TEMP_SIZE,
    .mem = ext_temp_mem,
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
    size_t alignment = EXT_ALIGN(size, EXT_DEFAULT_ALIGNMENT);
    intptr_t available = ext_temp_allocator.end - ext_temp_allocator.start - alignment;
    if(available < (intptr_t)size) {
#ifndef EXTLIB_NO_STD
        fprintf(stderr,
                "%s:%d: temp allocation failed: %zu bytes requested, %zd bytes "
                "available\n",
                __FILE__, __LINE__, size, available < 0 ? 0 : available);
        abort();
#else
        EXT_ASSERT(false, "temp allocation failed");
#endif
    }
    void *p = ext_temp_allocator.start;
    ext_temp_allocator.start += size + alignment;
    return p;
}

void *ext_temp_realloc(void *ptr, size_t old_size, size_t new_size) {
    ptrdiff_t alignment = EXT_ALIGN(old_size, EXT_DEFAULT_ALIGNMENT);
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

void *ext_temp_memdup(void *mem, size_t size) {
    void *new_mem = ext_temp_alloc(size);
    memcpy(new_mem, mem, size);
    return new_mem;
}

#ifndef EXTLIB_NO_STD
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

    EXT_ASSERT(n >= 0, "error in vsnprintf");
    char *res = ext_temp_alloc(n + 1);

    va_copy(cpy, ap);
    vsnprintf(res, n + 1, fmt, cpy);
    va_end(cpy);

    return res;
}
#endif  // EXTLIB_NO_STD
#endif  // EXTLIB_IMPL

// -----------------------------------------------------------------------------
// SECTION: Context
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
Ext_Context *ext_pop_context(void);

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

Ext_Context *ext_pop_context(void) {
    EXT_ASSERT(ext_context->prev, "Trying to pop default allocator");
    Ext_Context *old_ctx = ext_context;
    ext_context = old_ctx->prev;
    return old_ctx;
}
#endif

// -----------------------------------------------------------------------------
// SECTION: Arena allocator
//
typedef enum {
    EXT_ARENA_NONE = 0,
    // Forces the arena to behave like a stack allocator. The arena will
    // check that frees are done only in LIFO order. If this is not the
    // case, it will abort with an error.
    EXT_ARENA_STACK_ALLOC = 1 << 0,
    // Zeroes the memory allocated by the arena.
    EXT_ARENA_ZERO_ALLOC = 1 << 1,
    // When a single allocation requests more than `page_size` bytes, the arena
    // will request a larger page from the system instead of failing.
    EXT_ARENA_FLEXIBLE_PAGE = 1 << 2,
} Ext_ArenaFlags;

typedef struct Ext_ArenaPage {
    struct Ext_ArenaPage *next;
    char *start, *end;
    char data[];
} Ext_ArenaPage;

typedef struct Ext_Arena {
    Ext_Allocator base;
    const size_t alignment;
    const size_t page_size;
    Ext_Allocator *const page_allocator;
    Ext_ArenaFlags flags;
    Ext_ArenaPage *first_page, *last_page;
    size_t allocated;
} Ext_Arena;

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags);
void *ext_arena_alloc(Ext_Arena *a, size_t size);
void *ext_arena_realloc(Ext_Arena *a, void *ptr, size_t old_size, size_t new_size);
void ext_arena_free(Ext_Arena *a, void *ptr, size_t size);
void ext_arena_reset(Ext_Arena *a);
void ext_arena_destroy(Ext_Arena *a);
char *ext_arena_strdup(Ext_Arena *a, const char *str);
void *ext_arena_memdup(Ext_Arena *a, const void *mem, size_t size);
#ifndef EXTLIB_NO_STD
char *ext_arena_sprintf(Ext_Arena *a, const char *fmt, ...) EXT_PRINTF_FORMAT(2, 3);
char *ext_arena_vsprintf(Ext_Arena *a, const char *fmt, va_list ap);
#endif

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_ArenaFlags ArenaFlags;
typedef Ext_Arena Arena;
typedef Ext_ArenaPage ArenaPage;
#define new_arena      ext_new_arena
#define arena_alloc    ext_arena_alloc
#define arena_realloc  ext_arena_realloc
#define arena_free     ext_arena_free
#define arena_reset    ext_arena_reset
#define arena_destroy  ext_arena_destroy
#define arena_strdup   ext_arena_strdup
#define arena_memdup   ext_arena_memdup
#define arena_sprintf  ext_arena_sprintf
#define arena_vsprintf ext_arena_vsprintf
#endif  // EXTLIB_NO_SHORTHANDS

#ifdef EXTLIB_IMPL

#ifndef EXT_ARENA_PAGE_SZ
#define EXT_ARENA_PAGE_SZ (8 * 1024)  // 8 KiB
#endif                                // EXT_ARENA_PAGE_SZ

static Ext_ArenaPage *ext_arena_new_page(Ext_Arena *arena, size_t requested_size) {
    size_t header_sz = sizeof(Ext_ArenaPage) + EXT_ALIGN(sizeof(Ext_ArenaPage), arena->alignment);
    size_t actual_size = requested_size + header_sz;

    size_t page_size = arena->page_size;
    if(actual_size > page_size) {
        if(arena->flags & EXT_ARENA_FLEXIBLE_PAGE) {
            // TODO: maybe put flexible pages on another linked list?
            // this way we risk wasting space in pages coming before it in the linked list.
            page_size = actual_size;
        } else {
#ifndef EXTLIB_NO_STD
            fprintf(stderr,
                    "Error: requested size %zu exceeds max allocatable size in page "
                    "(%zu)\n",
                    requested_size, arena->page_size - header_sz);
            abort();
#else
            EXT_ASSERT(false, "reuqested size exceeds max allocatable size in page");
#endif
        }
    }

    Ext_ArenaPage *page = arena->page_allocator->alloc(arena->page_allocator, page_size);
    page->next = NULL;
    // Account for alignment of first allocation; the arena assumes every pointer
    // starts aligned to the arena's alignment.
    page->start = page->data + EXT_ALIGN(page->data, arena->alignment);
    page->end = (char *)page + page_size;
    return page;
}

static void *ext_arena_alloc_wrap(Ext_Allocator *a, size_t size) {
    return ext_arena_alloc((Ext_Arena *)a, size);
}

static void *ext_arena_realloc_wrap(Ext_Allocator *a, void *ptr, size_t old_size, size_t new_size) {
    return ext_arena_realloc((Ext_Arena *)a, ptr, old_size, new_size);
}

static void ext_arena_free_wrap(Ext_Allocator *a, void *ptr, size_t size) {
    ext_arena_free((Ext_Arena *)a, ptr, size);
}

Ext_Arena ext_new_arena(Ext_Allocator *page_alloc, size_t alignment, size_t page_size,
                        Ext_ArenaFlags flags) {
    if(!alignment) alignment = EXT_DEFAULT_ALIGNMENT;
    if(!page_size) page_size = EXT_ARENA_PAGE_SZ;
    EXT_ASSERT((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2");
    EXT_ASSERT(page_size > sizeof(Ext_ArenaPage) + EXT_ALIGN(sizeof(Ext_ArenaPage), alignment),
               "Page size must be greater the size of Ext_ArenaPage + alignment bytes");
    return (Ext_Arena){
        .base = {.alloc = ext_arena_alloc_wrap,
                 .realloc = ext_arena_realloc_wrap,
                 .free = ext_arena_free_wrap},
        .alignment = alignment,
        .page_size = page_size,
        .page_allocator = page_alloc ? page_alloc : ext_context->alloc,
        .flags = flags,
    };
}

void *ext_arena_alloc(Ext_Arena *a, size_t size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    size += EXT_ALIGN(size, arena->alignment);

    if(!arena->last_page) {
        EXT_ASSERT(arena->first_page == NULL && arena->allocated == 0,
                   "should be first allocation");
        Ext_ArenaPage *page = ext_arena_new_page(arena, size);
        arena->first_page = page;
        arena->last_page = page;
    }

    intptr_t available = arena->last_page->end - arena->last_page->start;
    while(available < (intptr_t)size) {
        Ext_ArenaPage *next_page = arena->last_page->next;
        if(!next_page) {
            arena->last_page->next = ext_arena_new_page(arena, size);
            arena->last_page = arena->last_page->next;
            available = arena->last_page->end - arena->last_page->start;
            break;
        } else {
            arena->last_page = next_page;
            available = next_page->end - next_page->start;
        }
    }

    EXT_ASSERT(available >= (intptr_t)size, "Not enough space in arena");

    void *p = arena->last_page->start;
    EXT_ASSERT(EXT_ALIGN(p, arena->alignment) == 0,
               "Pointer is not aligned to the arena's alignment");
    arena->last_page->start += size;
    arena->allocated += size;

    if(arena->flags & EXT_ARENA_ZERO_ALLOC) {
        memset(p, 0, size);
    }

    return p;
}

void *ext_arena_realloc(Ext_Arena *a, void *ptr, size_t old_size, size_t new_size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    EXT_ASSERT(EXT_ALIGN(ptr, arena->alignment) == 0,
               "ptr is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    EXT_ASSERT(page, "No pages in arena");

    size_t alignment = EXT_ALIGN(old_size, arena->alignment);
    if(page->start - old_size - alignment == ptr) {
        // Reallocating last allocated memory, can grow/shrink in-place
        page->start -= old_size + alignment;
        arena->allocated -= old_size + alignment;
        void *new_ptr = ext_arena_alloc(a, new_size);
        if(new_ptr != ptr) {
            memcpy(new_ptr, ptr, old_size);
        }
        return new_ptr;
    } else if(new_size > old_size) {
        void *new_ptr = ext_arena_alloc(a, new_size);
        memcpy(new_ptr, ptr, old_size);
        return new_ptr;
    } else {
        return ptr;
    }
}

void ext_arena_free(Ext_Arena *a, void *ptr, size_t size) {
    Ext_Arena *arena = (Ext_Arena *)a;
    EXT_ASSERT(EXT_ALIGN(ptr, arena->alignment) == 0,
               "ptr is not aligned to the arena's alignment");

    Ext_ArenaPage *page = arena->last_page;
    EXT_ASSERT(page, "No pages in arena");

    size_t alignment = EXT_ALIGN(size, arena->alignment);
    if(page->start - size - alignment == ptr) {
        // Deallocating last allocated memory, can shrink in-place
        page->start -= size + alignment;
        arena->allocated -= size + alignment;
        return;
    }

    // In stack allocator mode force LIFO order
    if(arena->flags & EXT_ARENA_STACK_ALLOC) {
#ifndef EXTLIB_NO_STD
        fprintf(stderr, "Deallocating memory in non-LIFO order: got %p, expected %p\n", ptr,
                (void *)(page->start - size - alignment));
        abort();
#else
        EXT_ASSERT(false, "Deallocating memory in non-LIFO order");
#endif
    }
}

void ext_arena_reset(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        page->start = page->data + EXT_ALIGN(page->data, a->alignment);
        page = page->next;
    }
    a->last_page = a->first_page;
    a->allocated = 0;
}

void ext_arena_destroy(Ext_Arena *a) {
    Ext_ArenaPage *page = a->first_page;
    while(page) {
        Ext_ArenaPage *next = page->next;
        a->page_allocator->free(a->page_allocator, page, page->end - (char *)page);
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

void *ext_arena_memdup(Ext_Arena *a, const void *mem, size_t size) {
    void *res = ext_arena_alloc(a, size);
    memcpy(res, mem, size);
    return res;
}

#ifndef EXTLIB_NO_STD
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

    EXT_ASSERT(n >= 0, "error in vsnprintf");
    char *res = ext_arena_alloc(a, n + 1);

    va_copy(cpy, ap);
    vsnprintf(res, n + 1, fmt, cpy);
    va_end(cpy);

    return res;
}
#endif  // EXTLIB_NO_STD
#endif  // EXTLIB_IMPL

// -----------------------------------------------------------------------------
// SECTION: Array
//
#ifndef EXT_ARRAY_INIT_CAP
#define EXT_ARRAY_INIT_CAP 8
#endif  // EXT_ARRAY_INIT_CAP

#define ext_array_foreach(T, it, vec) \
    for(T *it = (vec)->items, *end = (vec)->items + (vec)->size; it != end; it++)

#define ext_array_reserve(arr, newcap)                                                             \
    do {                                                                                           \
        if((arr)->capacity < (newcap)) {                                                           \
            size_t oldcap_ = (arr)->capacity;                                                      \
            (arr)->capacity = oldcap_ ? (arr)->capacity * 2 : EXT_ARRAY_INIT_CAP;                  \
            while((arr)->capacity < (newcap)) (arr)->capacity *= 2;                                \
            if(!((arr)->allocator)) (arr)->allocator = ext_context->alloc;                         \
            if(!(arr)->items) {                                                                    \
                (arr)->items = (arr)->allocator->alloc((arr)->allocator,                           \
                                                       (arr)->capacity * sizeof(*(arr)->items));   \
            } else {                                                                               \
                (arr)->items = (arr)->allocator->realloc((arr)->allocator, (arr)->items,           \
                                                         oldcap_ * sizeof(*(arr)->items),          \
                                                         (arr)->capacity * sizeof(*(arr)->items)); \
            }                                                                                      \
        }                                                                                          \
    } while(0)

#define ext_array_reserve_exact(arr, newcap)                                                       \
    do {                                                                                           \
        if((arr)->capacity < (newcap)) {                                                           \
            size_t oldcap_ = (arr)->capacity;                                                      \
            (arr)->capacity = newcap;                                                              \
            if(!((arr)->allocator)) (arr)->allocator = ext_context->alloc;                         \
            if(!(arr)->items) {                                                                    \
                (arr)->items = (arr)->allocator->alloc((arr)->allocator,                           \
                                                       (arr)->capacity * sizeof(*(arr)->items));   \
            } else {                                                                               \
                (arr)->items = (arr)->allocator->realloc((arr)->allocator, (arr)->items,           \
                                                         oldcap_ * sizeof(*(arr)->items),          \
                                                         (arr)->capacity * sizeof(*(arr)->items)); \
            }                                                                                      \
        }                                                                                          \
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

#define ext_array_push_all(a, elems, count)                                     \
    do {                                                                        \
        ext_array_reserve(a, (a)->size + (count));                              \
        memcpy((a)->items + (a)->size, (elems), (count) * sizeof(*(a)->items)); \
        (a)->size = (count);                                                    \
    } while(0)

#define ext_array_pop(a) (EXT_ASSERT((a)->size > 0, "no items to pop"), (a)->items[--(a)->size])

#define ext_array_remove(a, idx)                                            \
    do {                                                                    \
        EXT_ASSERT((size_t)(idx) < (a)->size, "array index out of bounds"); \
        if((size_t)(idx) < (a)->size - 1) {                                 \
            memmove((a)->items + (idx), (a)->items + (idx) + 1,             \
                    ((a)->size - idx - 1) * sizeof(*(a)->items));           \
        }                                                                   \
        (a)->size--;                                                        \
    } while(0)

#define ext_array_swap_remove(a, idx)                                       \
    do {                                                                    \
        EXT_ASSERT((size_t)(idx) < (a)->size, "array index out of bounds"); \
        if((size_t)(idx) < (a)->size - 1) {                                 \
            (a)->items[idx] = (a)->items[(a)->size - 1];                    \
        }                                                                   \
        (a)->size--;                                                        \
    } while(0)

#define ext_array_clear(a) \
    do {                   \
        (a)->size = 0;     \
    } while(0)

#define ext_array_resize(a, new_sz)             \
    do {                                        \
        ext_array_reserve_exact((a), (new_sz)); \
        (a)->size = new_sz;                     \
    } while(0)

#define ext_array_shrink_to_fit(a)                                                        \
    do {                                                                                  \
        if((a)->capacity > (a)->size) {                                                   \
            if((a)->size == 0) {                                                          \
                (a)->allocator->free((a)->allocator, (a)->items,                          \
                                     (a)->capacity * sizeof(*(a)->items));                \
                (a)->items = NULL;                                                        \
            } else {                                                                      \
                (a)->items = (a)->allocator->realloc((a)->allocator, (a)->items,          \
                                                     (a)->capacity * sizeof(*(a)->items), \
                                                     (a)->size * sizeof(*(a)->items));    \
            }                                                                             \
            (a)->capacity = (a)->size;                                                    \
        }                                                                                 \
    } while(0);

#ifndef EXTLIB_NO_SHORTHANDS
#define array_foreach       ext_array_foreach
#define array_reserve       ext_array_reserve
#define array_reserve_exact ext_array_reserve_exact
#define array_push          ext_array_push
#define array_free          ext_array_free
#define array_push_all      ext_array_push_all
#define array_pop           ext_array_pop
#define array_remove        ext_array_remove
#define array_swap_remove   ext_array_swap_remove
#define array_clear         ext_array_clear
#define array_resize        ext_array_resize
#define array_shrink_to_fit ext_array_shrink_to_fit
#endif  // EXT_NO_SHORTHANDS

// -----------------------------------------------------------------------------
// SECTION: Stringbuffer
//

typedef struct {
    char *items;
    size_t capacity, size;
    Ext_Allocator *allocator;
} Ext_StringBuffer;

#define ext_sb_free(sb)              ext_array_free(sb)
#define ext_sb_append_char(sb, c)    ext_array_push(sb, c)
#define ext_sb_append(sb, mem, size) ext_array_push_all(sb, mem, size)
#define ext_sb_append_cstr(sb, str)       \
    do {                                  \
        const char *s_ = (str);           \
        size_t len_ = strlen(s_);         \
        ext_array_push_all(sb, s_, len_); \
    } while(0)

#define ext_sb_prepend(sb, mem, count)                         \
    do {                                                       \
        ext_array_reserve(sb, (sb)->size + (count));           \
        memmove((sb)->items + count, (sb)->items, (sb)->size); \
        memcpy((sb)->items, mem, count);                       \
        (sb)->size += count;                                   \
    } while(0)

#define ext_sb_prepend_cstr(sb, str)  \
    do {                              \
        const char *s_ = (str);       \
        size_t len_ = strlen(str);    \
        ext_sb_prepend(sb, s_, len_); \
    } while(0)

#define ext_sb_prepend_char(sb, c)  \
    do {                            \
        char c_ = (c);              \
        ext_sb_prepend(sb, &c_, 1); \
    } while(0)

void ext_sb_replace(Ext_StringBuffer *sb, size_t start, const char *to_replace, char replacment);
char *ext_sb_to_cstr(Ext_StringBuffer *sb);
#ifndef EXTLIB_NO_STD
int ext_sb_appendf(Ext_StringBuffer *sb, const char *fmt, ...) EXT_PRINTF_FORMAT(2, 3);
int ext_sb_appendvf(Ext_StringBuffer *sb, const char *fmt, va_list ap);
#endif  // EXTLIB_NO_STD

#define Ext_SB_Fmt     "%.*s"
#define Ext_SB_Arg(ss) (int)ss.size, ss.items

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_StringBuffer StringBuffer;
#define sb_free         ext_sb_free
#define sb_append_char  ext_sb_append_char
#define sb_append       ext_sb_append
#define sb_append_cstr  ext_sb_append_cstr
#define sb_prepend      ext_sb_prepend
#define sb_prepend_cstr ext_sb_prepend_cstr
#define sb_prepend_char ext_sb_prepend_char
#define sb_replace      ext_sb_replace
#define sb_to_cstr      ext_sb_to_cstr
#ifndef EXTLIB_NO_STD
#define sb_appendf  ext_sb_appendf
#define sb_appendvf ext_sb_appendvf
#endif  // EXTLIB_NO_STD
#define SB_Fmt Ext_SB_Fmt
#define SB_Arg Ext_SB_Arg
#endif  // EXTLIB_NO_SHORTHANDS

#ifdef EXTLIB_IMPL
void ext_sb_replace(Ext_StringBuffer *sb, size_t start, const char *to_replace, char replacment) {
    EXT_ASSERT(start < sb->size, "start out of bounds");
    size_t to_replace_len = strlen(to_replace);
    for(size_t i = start; i < sb->size; i++) {
        for(size_t j = 0; j < to_replace_len; j++) {
            if(sb->items[i] == to_replace[j]) {
                sb->items[i] = replacment;
                break;
            }
        }
    }
}

char *ext_sb_to_cstr(Ext_StringBuffer *sb) {
    ext_sb_append_char(sb, '\0');
    ext_array_shrink_to_fit(sb);
    char *res = sb->items;
    *sb = (Ext_StringBuffer){0};
    return res;
}

#ifndef EXTLIB_NO_STD
int ext_sb_appendf(Ext_StringBuffer *sb, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int res = ext_sb_appendvf(sb, fmt, ap);
    va_end(ap);
    return res;
}

int ext_sb_appendvf(Ext_StringBuffer *sb, const char *fmt, va_list ap) {
    va_list cpy;
    va_copy(cpy, ap);
    int n = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);

    EXT_ASSERT(n >= 0, "error in vsnprintf");
    ext_array_reserve(sb, sb->size + n + 1);

    va_copy(cpy, ap);
    vsnprintf(sb->items + sb->size, n + 1, fmt, cpy);
    va_end(cpy);

    sb->size += n;
    return n;
}

#endif  // EXTLIB_NO_STD
#endif  // EXTLIB_IMPL

// -----------------------------------------------------------------------------
// SECTION: String slice
//

typedef struct {
    size_t size;
    const char *data;
} Ext_StringSlice;

Ext_StringSlice ext_ss_from(const void *mem, size_t size);
Ext_StringSlice ext_ss_from_cstr(const char *str);
Ext_StringSlice ext_ss_split_once(Ext_StringSlice *ss, char delim);
Ext_StringSlice ext_ss_rsplit_once(Ext_StringSlice *ss, char delim);
Ext_StringSlice ext_ss_split_once_ws(Ext_StringSlice *ss);
Ext_StringSlice ext_ss_trim_start(Ext_StringSlice ss);
Ext_StringSlice ext_ss_trim_end(Ext_StringSlice ss);
Ext_StringSlice ext_ss_trim(Ext_StringSlice ss);
Ext_StringSlice ext_ss_cut(Ext_StringSlice ss, size_t n);
Ext_StringSlice ext_ss_trunc(Ext_StringSlice ss, size_t n);
bool ext_ss_starts_with(Ext_StringSlice ss, Ext_StringSlice prefix);
bool ext_ss_ends_with(Ext_StringSlice ss, Ext_StringSlice suffix);
int ext_ss_cmp(Ext_StringSlice s1, Ext_StringSlice s2);
bool ext_ss_eq(Ext_StringSlice s1, Ext_StringSlice s2);
#define ext_sb_to_ss(sb) (ext_ss_from((sb).items, (sb).size))

#define Ext_SS_Fmt     "%.*s"
#define Ext_SS_Arg(ss) (int)ss.size, ss.data

#ifndef EXTLIB_NO_SHORTHANDS
typedef Ext_StringSlice StringSlice;
#define ss_from          ext_ss_from
#define ss_from_cstr     ext_ss_from_cstr
#define ss_split_once    ext_ss_split_once
#define ss_rsplit_once   ext_ss_rsplit_once
#define ss_split_once_ws ext_ss_split_once_ws
#define ss_trim_start    ext_ss_trim_start
#define ss_cut           ext_ss_cut
#define ss_trunc         ext_ss_trunc
#define ss_starts_with   ext_ss_starts_with
#define ss_ends_with     ext_ss_ends_with
#define ss_trim_end      ext_ss_trim_end
#define ss_trim          ext_ss_trim
#define ss_cmp           ext_ss_cmp
#define ss_eq            ext_ss_eq
#define sb_to_ss         ext_sb_to_ss
#define SS_Fmt           Ext_SS_Fmt
#define SS_Arg           Ext_SS_Arg
#endif

#ifdef EXTLIB_IMPL

#ifndef EXTLIB_NO_STD
#include <ctype.h>
#else
static inline int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}
#endif  // EXTLIB_NO_STD

Ext_StringSlice ext_ss_from(const void *mem, size_t size) {
    return (Ext_StringSlice){size, mem};
}

Ext_StringSlice ext_ss_from_cstr(const char *str) {
    return ext_ss_from(str, strlen(str));
}

Ext_StringSlice ext_ss_split_once(Ext_StringSlice *ss, char delim) {
    Ext_StringSlice split = {0, ss->data};
    while(ss->size) {
        ss->size--;
        if(*ss->data++ == delim) break;
        else split.size++;
    }
    return split;
}

Ext_StringSlice ext_ss_rsplit_once(Ext_StringSlice *ss, char delim) {
    Ext_StringSlice split = {0, ss->data + ss->size};
    while(ss->size) {
        ss->size--;
        if(ss->data[ss->size] == delim) break;
        else split.size++, split.data--;
    }
    return split;
}

Ext_StringSlice ext_ss_split_once_ws(Ext_StringSlice *ss) {
    Ext_StringSlice split = {0, ss->data};
    while(ss->size) {
        ss->size--;
        if(isspace(*ss->data++)) break;
        else split.size++;
    }
    while(ss->size && isspace(*ss->data)) {
        ss->data++;
        ss->size--;
    }
    return split;
}

Ext_StringSlice ext_ss_trim_start(Ext_StringSlice ss) {
    while(ss.size && isspace(*ss.data)) ss.data++, ss.size--;
    return ss;
}

Ext_StringSlice ext_ss_trim_end(Ext_StringSlice ss) {
    while(ss.size && isspace(ss.data[ss.size - 1])) ss.size--;
    return ss;
}

Ext_StringSlice ext_ss_trim(Ext_StringSlice ss) {
    return ext_ss_trim_end(ext_ss_trim_start(ss));
}

Ext_StringSlice ext_ss_cut(Ext_StringSlice ss, size_t n) {
    if(n > ss.size) n = ss.size;
    ss.data += n;
    ss.size -= n;
    return ss;
}

Ext_StringSlice ext_ss_trunc(Ext_StringSlice ss, size_t n) {
    if(n > ss.size) n = ss.size;
    ss.size -= ss.size - n;
    return ss;
}

bool ext_ss_starts_with(Ext_StringSlice ss, Ext_StringSlice prefix) {
    return prefix.size <= ss.size && memcmp(ss.data, prefix.data, prefix.size) == 0;
}

bool ext_ss_ends_with(Ext_StringSlice ss, Ext_StringSlice suffix) {
    return suffix.size <= ss.size &&
           memcmp(&ss.data[ss.size - suffix.size], suffix.data, suffix.size) == 0;
}

int ext_ss_cmp(Ext_StringSlice s1, Ext_StringSlice s2) {
    size_t min_sz = s1.size < s2.size ? s1.size : s2.size;
    return memcmp(s1.data, s2.data, min_sz);
}

bool ext_ss_eq(Ext_StringSlice s1, Ext_StringSlice s2) {
    return s1.size == s2.size && memcmp(s1.data, s2.data, s1.size) == 0;
}
#endif

// -----------------------------------------------------------------------------
// SECTION: Hashmap
//

// Read as: size * 0.75, i.e. a load factor of 75%
// This is basically doing:
//   size / 2 + size / 4 = (3 * size) / 4
#define EXT_HMAP_MAX_ENTRY_LOAD(size) (((size) >> 1) + ((size) >> 2))
#define EXT_HMAP_EMPTY_MARK           0
#define EXT_HMAP_TOMB_MARK            1
#ifndef EXT_HMAP_INIT_CAPACITY
#define EXT_HMAP_INIT_CAPACITY 8
#endif  // EXT_HMAP_INIT_CAPACITY

EXT_STATIC_ASSERT(((EXT_HMAP_INIT_CAPACITY) & (EXT_HMAP_INIT_CAPACITY - 1)) == 0,
                  "hashmap initial capacity must be a power of two");

#define EXT_HMAP_IS_TOMB(h)  ((h) == EXT_HMAP_TOMB_MARK)
#define EXT_HMAP_IS_EMPTY(h) ((h) == EXT_HMAP_EMPTY_MARK)
#define EXT_HMAP_IS_VALID(h) (!EXT_HMAP_IS_EMPTY(h) && !EXT_HMAP_IS_TOMB(h))

#define ext_hmap_foreach(T, it, hmap)                                       \
    for(T *it = ext_hmap_begin(hmap), *end = ext_hmap_end(hmap); it != end; \
        it = ext_hmap_next(hmap, it))

#define ext_hmap_end(hmap) \
    ext_hmap_end_((hmap)->entries, (hmap)->capacity, sizeof(*(hmap)->entries))
#define ext_hmap_begin(hmap) \
    ext_hmap_begin_((hmap)->entries, (hmap)->hashes, (hmap)->capacity, sizeof(*(hmap)->entries))
#define ext_hmap_next(hmap, it) \
    ext_hmap_next_((hmap)->entries, (hmap)->hashes, it, (hmap)->capacity, sizeof(*(hmap)->entries))

#define ext_hmap_put_ex(hmap, entry, hash, cmp)                             \
    do {                                                                    \
        if((hmap)->size >= EXT_HMAP_MAX_ENTRY_LOAD((hmap)->capacity + 1)) { \
            ext_hmap_grow_(hmap);                                           \
        }                                                                   \
        size_t hash_ = hash(entry);                                         \
        if(hash_ < 2) hash_ += 2;                                           \
        ext_hmap_find_index_(hmap, entry, hash_, cmp);                      \
        if(!EXT_HMAP_IS_VALID((hmap)->hashes[idx_])) {                      \
            (hmap)->size++;                                                 \
        }                                                                   \
        (hmap)->hashes[idx_] = hash_;                                       \
        (hmap)->entries[idx_] = *(entry);                                   \
    } while(0)

#define ext_hmap_get_ex(hmap, entry, out, hash, cmp)   \
    do {                                               \
        size_t hash_ = hash(entry);                    \
        if(hash_ < 2) hash_ += 2;                      \
        ext_hmap_find_index_(hmap, entry, hash_, cmp); \
        if(EXT_HMAP_IS_VALID((hmap)->hashes[idx_])) {  \
            *(out) = &(hmap)->entries[idx_];           \
        } else {                                       \
            *(out) = NULL;                             \
        }                                              \
    } while(0)

#define ext_hmap_delete_ex(hmap, entry, hash, cmp)     \
    do {                                               \
        size_t hash_ = hash(entry);                    \
        if(hash_ < 2) hash_ += 2;                      \
        ext_hmap_find_index_(hmap, entry, hash_, cmp); \
        if(EXT_HMAP_IS_VALID((hmap)->hashes[idx_])) {  \
            (hmap)->hashes[idx_] = EXT_HMAP_TOMB_MARK; \
            (hmap)->size--;                            \
        }                                              \
    } while(0)

#define ext_hmap_put(hmap, entry) \
    ext_hmap_put_ex(hmap, entry, ext_hmap_hash_bytes_, ext_hmap_memcmp_)
#define ext_hmap_get(hmap, entry, out) \
    ext_hmap_get_ex(hmap, entry, out, ext_hmap_hash_bytes_, ext_hmap_memcmp_)
#define ext_hmap_delete(hmap, entry) \
    ext_hmap_delete_ex(hmap, entry, ext_hmap_hash_bytes_, ext_hmap_memcmp_)

#define ext_hmap_put_cstr(hmap, entry) \
    ext_hmap_put_ex(hmap, entry, ext_hmap_hash_cstr_entry_, ext_hmap_strcmp_entry_)
#define ext_hmap_get_cstr(hmap, entry, out) \
    ext_hmap_get_ex(hmap, entry, out, ext_hmap_hash_cstr_, ext_hmap_strcmp_)
#define ext_hmap_delete_cstr(hmap, entry) \
    ext_hmap_delete_ex(hmap, entry, ext_hmap_hash_cstr_, ext_hmap_strcmp_)

#define ext_hmap_put_ss(hmap, entry) \
    ext_hmap_put_ex(hmap, entry, ext_hmap_hash_ss_entry_, ext_hmap_sscmp_entry_)
#define ext_hmap_get_ss(hmap, entry, out) \
    ext_hmap_get_ex(hmap, entry, out, ext_hmap_hash_ss_, ext_hmap_sscmp_)
#define ext_hmap_delete_ss(hmap, entry) \
    ext_hmap_delete_ex(hmap, entry, ext_hmap_hash_ss_, ext_hmap_sscmp_)

#define ext_hmap_clear(hmap)                                                         \
    do {                                                                             \
        memset((hmap)->hashes, 0, sizeof(*(hmap)->hashes) * ((hmap)->capacity + 1)); \
        (hmap)->size = 0;                                                            \
    } while(0)

#define ext_hmap_free(hmap)                 \
    do {                                    \
        ext_hmap_free_(hmap);               \
        memset((hmap), 0, sizeof(*(hmap))); \
    } while(0)

#define ext_hmap_grow_(hmap)                                                                    \
    do {                                                                                        \
        size_t newcap = (hmap)->capacity ? ((hmap)->capacity + 1) * 2 : EXT_HMAP_INIT_CAPACITY; \
        size_t newsz = newcap * sizeof(*(hmap)->entries);                                       \
        size_t pad = EXT_ALIGN(newsz, sizeof(*(hmap)->hashes));                                 \
        size_t totalsz = newsz + pad + sizeof(*(hmap)->hashes) * newcap;                        \
        if(!(hmap)->allocator) {                                                                \
            (hmap)->allocator = ext_context->alloc;                                             \
        }                                                                                       \
        void *newentries = (hmap)->allocator->alloc((hmap)->allocator, totalsz);                \
        size_t *newhashes = (size_t *)((char *)newentries + newsz + pad);                       \
        EXT_ASSERT(((uintptr_t)newhashes & (sizeof(*(hmap)->hashes) - 1)) == 0,                 \
                   "newhashes allocation is not aligned");                                      \
        memset(newhashes, 0, sizeof(*(hmap)->hashes) * newcap);                                 \
        if((hmap)->capacity > 0) {                                                              \
            for(size_t i = 0; i <= (hmap)->capacity; i++) {                                     \
                size_t hash = (hmap)->hashes[i];                                                \
                if(EXT_HMAP_IS_VALID(hash)) {                                                   \
                    size_t newidx = hash & (newcap - 1);                                        \
                    while(!EXT_HMAP_IS_EMPTY(newhashes[newidx])) {                              \
                        newidx = (newidx + 1) & (newcap - 1);                                   \
                    }                                                                           \
                    memcpy((char *)newentries + newidx * sizeof(*(hmap)->entries),              \
                           (hmap)->entries + i, sizeof(*(hmap)->entries));                      \
                    newhashes[newidx] = hash;                                                   \
                }                                                                               \
            }                                                                                   \
        }                                                                                       \
        ext_hmap_free_(hmap);                                                                   \
        (hmap)->entries = newentries;                                                           \
        (hmap)->hashes = newhashes;                                                             \
        (hmap)->capacity = newcap - 1;                                                          \
    } while(0)

#define ext_hmap_find_index_(map, entry, hash, cmp)                             \
    size_t idx_ = 0;                                                            \
    {                                                                           \
        size_t i_ = (hash) & (map)->capacity;                                   \
        bool tomb_found_ = false;                                               \
        size_t tomb_idx_ = 0;                                                   \
        for(;;) {                                                               \
            size_t buck = (map)->hashes[i_];                                    \
            if(!EXT_HMAP_IS_VALID(buck)) {                                      \
                if(EXT_HMAP_IS_EMPTY(buck)) {                                   \
                    idx_ = tomb_found_ ? tomb_idx_ : i_;                        \
                    break;                                                      \
                } else if(!tomb_found_) {                                       \
                    tomb_found_ = true;                                         \
                    tomb_idx_ = i_;                                             \
                }                                                               \
            } else if(buck == hash && cmp((entry), &(map)->entries[i_]) == 0) { \
                idx_ = i_;                                                      \
                break;                                                          \
            }                                                                   \
            i_ = (i_ + 1) & (map)->capacity;                                    \
        }                                                                       \
    }

#define ext_hmap_free_(hmap)                                                              \
    do {                                                                                  \
        if((hmap)->entries) {                                                             \
            size_t sz = ((hmap)->capacity + 1) * sizeof(*(hmap)->entries);                \
            size_t pad = EXT_ALIGN(sz, sizeof(*(hmap)->hashes));                          \
            size_t totalsz = sz + pad + sizeof(*(hmap)->hashes) * ((hmap)->capacity + 1); \
            (hmap)->allocator->free((hmap)->allocator, (hmap)->entries, totalsz);         \
        }                                                                                 \
    } while(0)

#define ext_hmap_hash_bytes_(e)      ext_hash_bytes_(&(e)->key, sizeof((e)->key))
#define ext_hmap_hash_cstr_entry_(e) ext_hash_cstr_((e)->key)
#define ext_hmap_hash_cstr_(e)       ext_hash_cstr_(e)
#define ext_hmap_hash_ss_entry_(e)   ext_hash_bytes_((e)->key.data, (e)->key.size)
#define ext_hmap_hash_ss_(e)         ext_hash_bytes_((e).data, (e).size)
#define ext_hmap_memcmp_(a, b)       memcmp(&(a)->key, &(b)->key, sizeof((a)->key))
#define ext_hmap_strcmp_entry_(a, b) strcmp((a)->key, (b)->key)
#define ext_hmap_strcmp_(a, b)       strcmp((a), (b)->key)
#define ext_hmap_sscmp_entry_(a, b)  ext_ss_cmp((a)->key, (b)->key)
#define ext_hmap_sscmp_(a, b)        ext_ss_cmp((a), (b)->key)

static inline void *ext_hmap_end_(const void *entries, size_t cap, size_t sz) {
    return entries ? (char *)entries + (cap + 1) * sz : NULL;
}

static inline void *ext_hmap_begin_(const void *entries, const size_t *hashes, size_t cap,
                                    size_t sz) {
    if(!entries) return NULL;
    for(size_t i = 0; i <= cap; i++) {
        if(EXT_HMAP_IS_VALID(hashes[i])) {
            return (char *)entries + i * sz;
        }
    }
    return ext_hmap_end_(entries, cap, sz);
}

static inline void *ext_hmap_next_(const void *entries, const size_t *hashes, const void *it,
                                   size_t cap, size_t sz) {
    size_t curr = ((char *)it - (char *)entries) / sz;
    for(size_t idx = curr + 1; idx <= cap; idx++) {
        if(EXT_HMAP_IS_VALID(hashes[idx])) {
            return (char *)entries + idx * sz;
        }
    }
    return ext_hmap_end_(entries, cap, sz);
}

#ifndef EXTLIB_NO_SHORTHANDS
#define hmap_foreach     ext_hmap_foreach
#define hmap_end         ext_hmap_end
#define hmap_begin       ext_hmap_begin
#define hmap_next        ext_hmap_next
#define hmap_put         ext_hmap_put
#define hmap_get         ext_hmap_get
#define hmap_delete      ext_hmap_delete
#define hmap_put_cstr    ext_hmap_put_cstr
#define hmap_get_cstr    ext_hmap_get_cstr
#define hmap_delete_cstr ext_hmap_delete_cstr
#define hmap_put_ss      ext_hmap_put_ss
#define hmap_get_ss      ext_hmap_get_ss
#define hmap_delete_ss   ext_hmap_delete_ss
#define hmap_clear       ext_hmap_clear
#define hmap_free        ext_hmap_free
#endif  // EXTLIB_NO_SHORTHANDS

// ============================================================================
// Taken from stb_ds.h
// TODO: add stb_ds license
// TODO: put under EXTLIB_IMPLEMENTATION flag

#define EXT_SIZET_BITS           ((sizeof(size_t)) * CHAR_BIT)
#define EXT_ROTATE_LEFT(val, n)  (((val) << (n)) | ((val) >> (EXT_SIZET_BITS - (n))))
#define EXT_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (EXT_SIZET_BITS - (n))))

static inline size_t ext_hash_cstr_(const char *str) {
    const size_t seed = 2147483647;
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
#pragma warning(disable : 4127)  // conditional expression is constant, for
                                 // do..while(0) and sizeof()==
#endif

static size_t stbds_siphash_bytes(const void *p, size_t len, size_t seed) {
    unsigned char *d = (unsigned char *)p;
    size_t i, j;
    size_t v0, v1, v2, v3, data;

    // hash that works on 32- or 64-bit registers without knowing which we have
    // (computes different results on 32-bit and 64-bit platform)
    // derived from siphash, but on 32-bit platforms very different as it uses 4
    // 32-bit state not 4 64-bit
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
    return v1 ^ v2 ^ v3;  // slightly stronger since v0^v3 in above cancels out
                          // final round operation? I tweeted at the authors of
                          // SipHash about this but they didn't reply
#endif
}

static inline size_t ext_hash_bytes_(const void *p, size_t len) {
    const size_t seed = 2147483647;
#ifdef STBDS_SIPHASH_2_4
    return stbds_siphash_bytes(p, len, seed);
#else
    unsigned char *d = (unsigned char *)p;

    if(len == 4) {
        unsigned int hash = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash
        // with rotates turned into shifts. Note that converting these back to
        // rotates makes it run a lot slower, presumably due to collisions, so I'm
        // not really sure what's going on.
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

// static int key_equals(void *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset,
//                       int mode, size_t i) {
//     // if(mode >= STBDS_HM_STRING)
//     //     return 0 == strcmp((char *)key, *(char **)((char *)a + elemsize * i +
//     //     keyoffset));
//     // else rreturn 0 == memcmp(key, (char *)a + elemsize * i + keyoffset,
//     // keysize);
//     return 0 == memcmp(key, (char *)a + elemsize * i + keyoffset, keysize);
// }

// End of stbds.h
// =============================================================================
#endif  // EXTLIB_H
