#ifndef EXT_CONTEXT
#define EXT_CONTEXT

#ifndef EXTLIB_NO_CONTEXT_THREAD_SAFE
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

#define EXT_PUSH_CONTEXT(ctx, code) \
    do {                            \
        push_context(ctx);          \
        code;                       \
        pop_context();              \
    } while(0)

struct Ext_Allocator;

typedef struct Ext_Context {
    struct Ext_Allocator* alloc;
    struct Ext_Context* prev;
} Ext_Context;
extern EXT_TLS Ext_Context* ext_context;

void push_context(Ext_Context* ctx);
Ext_Context* pop_context();

#endif
