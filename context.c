#include <assert.h>

#include "alloc.h"
#include "context.h"

EXT_TLS Ext_Context* ext_context = &(Ext_Context){
    .alloc = (Ext_Allocator*)&ext_default_allocator,
};

void ext_push_context(Ext_Context* ctx) {
    ctx->prev = ext_context;
    ext_context = ctx;
}

Ext_Context* ext_pop_context() {
    assert(ext_context->prev && "Trying to pop default allocator");
    Ext_Context* old_ctx = ext_context;
    ext_context = old_ctx->prev;
    return old_ctx;
}
