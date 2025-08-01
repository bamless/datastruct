#include <stddef.h>

#ifndef EXTLIB_WASM
#include <stdio.h>
#else
int printf(const char* fmt, ...);
#endif

#define EXTLIB_IMPL
#include "../extlib.h"

const char* prec[] = {
    "+-",
    "*/",
};
const size_t max_prec = ARR_SIZE(prec);

size_t get_prec(char op) {
    for(size_t i = 0; i < ARR_SIZE(prec); i++) {
        for(size_t j = 0; j < strlen(prec[i]); j++) {
            if(prec[i][j] == op)  return i;
        }
    }
    return max_prec;
}

typedef struct {
    const char* src;
    const char* ptr;
} Src;

typedef struct {
    char op;
    struct Expr *l, *r;
} BinExpr;

typedef struct Expr {
    enum { BIN, LIT } kind;
    union {
        BinExpr bin;
        int lit;
    } as;
} Expr;

void error(const Src* src, const char* msg) {
    int col = src->ptr - src->src;
    ASSERT(col >= 0, "negative column");
    printf("%d: ", col);
    printf("%s\n", msg);
    printf("%s\n", src->src);
    for(int i = 1; i < col; i++) {
        printf(" ");
    }
    printf("^\n");
}

static int is_space(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

char advance(Src* src) {
    while(is_space(*src->ptr)) src->ptr++;
    return *src->ptr++;
}

char peek(const Src* src) {
    Src s = *src;
    return advance(&s);
}

Expr* parse_bin_expr(Src* expr, size_t prec, Arena* a);

Expr* parse_lit(Src* src, Arena* a) {
    char c = advance(src);

    if(c == '(') {
        Expr* e = parse_bin_expr(src, 0, a);
        if(!e) return NULL;

        c = advance(src);
        if(c != ')') {
            error(src, "expected ')'");
            return NULL;
        }

        return e;
    }

    int sign = 1;
    if(c == '-') {
        sign = -1;
        c = advance(src);
    }

    if(c < '0' || c > '9') {
        error(src, "Expected digit");
        return NULL;
    }

    int val = c - '0';
    if(c != '0') {
        while(*src->ptr >= '0' && *src->ptr <= '9') {
            val = val * 10 + (*src->ptr++ - '0');
        }
    }

    Expr* lit = arena_alloc(a, sizeof(Expr));
    lit->kind = LIT;
    lit->as.lit = sign * val;
    return lit;
}

Expr* parse_bin_expr(Src* src, size_t prec, Arena* a) {
    if(prec >= max_prec) return parse_lit(src, a);

    Expr* l = parse_bin_expr(src, prec + 1, a);
    if(!l) return NULL;

    char op;
    while((op = peek(src)) && get_prec(op) == prec) {
        advance(src);

        Expr* r = parse_bin_expr(src, prec + 1, a);
        if(!r) return NULL;

        Expr* bin = ext_arena_alloc(a, sizeof(Expr));
        bin->kind = BIN;
        bin->as.bin = (BinExpr){op, l, r};
        l = bin;
    }

    return l;
}

Expr* parse_expr(const char* expr, Arena* a) {
    Src src = {expr, expr};
    Expr* res = parse_bin_expr(&src, 0, a);
    if(!res) return NULL;

    char c;
    if((c = advance(&src)) != '\0') {
        error(&src, "Unexpected token");
        return NULL;
    }

    return res;
}

double interpret_expr(const Expr* e) {
    ASSERT(e, "e is NULL");
    switch(e->kind) {
    case BIN: {
        double l = interpret_expr(e->as.bin.l);
        double r = interpret_expr(e->as.bin.r);
        switch(e->as.bin.op) {
        case '+': return l + r;
        case '-': return l - r;
        case '*': return l * r;
        case '/': return l / r;
        default:  UNREACHABLE();
        }
    }
    case LIT: return e->as.lit;
    }
    UNREACHABLE();
}

void dump_expr(const Expr* e) {
    ASSERT(e, "e is NULL");
    switch(e->kind) {
    case BIN:
        printf("(");
        dump_expr(e->as.bin.l);
        printf(" %c ", e->as.bin.op);
        dump_expr(e->as.bin.r);
        printf(")");
        break;
    case LIT:
        printf("%d", e->as.lit);
        break;
    }
}

#ifndef EXTLIB_WASM
int main(void) {
    // Arenas are pretty good when dealing with lots of small allocations,
    // such as trees or linked lists
    Arena a = new_arena(NULL, 0, 0, 0);

    Expr* expr = parse_expr(" 1   + 2 /  4 + 3 - 4 * 2  ", &a);
    if(!expr) {
        arena_destroy(&a);
        return 1;
    }

    dump_expr(expr);
    printf(" = ");
    printf("%g\n", interpret_expr(expr));

    // free all at once!
    arena_destroy(&a);
}
#else
double eval_expr(char* src) {
    Arena a = new_arena(&ext_temp_allocator.base, 0, 0, 0);

    Expr* expr = parse_expr(src, &a);
    if(!expr) return 0./0.; // NaN
    double res = interpret_expr(expr);

    arena_destroy(&a);
    return res;
}
#endif
