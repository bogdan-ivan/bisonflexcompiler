#include "AST.h"

#include <stdarg.h>
#include <stdlib.h>

#include "buffer.h"

ASTNode* oper(int type, int nops, ...) {
    va_list args;
    ASTNode* node;

    node = malloc(sizeof *node);
    node->type = OPER;
    node->oper.type = type;
    node->oper.ops = NULL;

    va_start(args, nops);
    for (int i = 0; i < nops; ++i) {
        buf_push(node->oper.ops, va_arg(args, ASTNode*));
    }
    va_end(args);

    return node;
}
ASTNode* id(char *name) {
    ASTNode* node;

    node = malloc(sizeof *node);
    node->type = ID;
    node->id.name = name;

    return node;
}
ASTNode* literal(int value) {
    ASTNode* node;

    node = malloc(sizeof *node);
    node->type = LITERAL;
    node->literal.value = value;

    return node;
}
void free_AST(ASTNode* node) {
    if (NULL == node) return;

    if (OPER == node->type) {
        buf_range(i, node->oper.ops) {
            free_AST(node->oper.ops[i]);
        }
        buf_free(node->oper.ops);
    } else if (ID == node->type) {
        free(node->id.name);
    }
    free(node);
}