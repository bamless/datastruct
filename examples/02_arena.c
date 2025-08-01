#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXTLIB_IMPL
#include "../extlib.h"

const char* prec[] = {
    "+-",
    "*/",
};
const size_t max_prec = ARR_SIZE(prec);

size_t get_prec(char op) {
    for(size_t i = 0; i < ARR_SIZE(prec); i++) {
        if(strchr(prec[i], op) != NULL) return i;
    }
    return max_prec;
}

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

char advance(const char** expr) {
    while(isspace(**expr)) (*expr)++;
    if(**expr == '\0') return '\0';
    return *(*expr)++;
}

char peek(const char* expr) {
    return advance(&expr);
}

Expr* parse_bin_expr(const char** expr, size_t prec, Arena* a);

Expr* parse_lit(const char** expr, Arena* a) {
    char c = advance(expr);

    if(c == '(') {
        Expr* e = parse_bin_expr(expr, 0, a);
        if(!e) return NULL;
        c = advance(expr);
        if(c != ')') {
            ext_log(ERROR, "expected ')', found %.*s\n", 1, c == 0 ? "0" : &c);
            return NULL;
        }
        return e;
    }

    if(c < '0' || c > '9') {
        ext_log(ERROR, "Unexpected token %c\n", c);
        return NULL;
    }

    Expr* lit = arena_alloc(a, sizeof(Expr));
    lit->kind = LIT;
    lit->as.lit = c - '0';
    return lit;
}

Expr* parse_bin_expr(const char** expr, size_t prec, Arena* a) {
    if(prec >= max_prec) return parse_lit(expr, a);

    Expr* l = parse_bin_expr(expr, prec + 1, a);
    if(!l) return NULL;

    char op;
    while((op = peek(*expr)) && get_prec(op) == prec) {
        advance(expr);

        Expr* r = parse_bin_expr(expr, prec + 1, a);
        if(!r) return NULL;

        Expr* bin = ext_arena_alloc(a, sizeof(Expr));
        bin->kind = BIN;
        bin->as.bin = (BinExpr){op, l, r};
        l = bin;
    }

    return l;
}

Expr* parse_expr(const char* expr, Arena* a) {
    Expr* res = parse_bin_expr(&expr, 0, a);
    if(!res) return NULL;
    char c;
    if((c = advance(&expr)) != '\0') {
        ext_log(ERROR, "Unexpected token: %c\n", c);
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
