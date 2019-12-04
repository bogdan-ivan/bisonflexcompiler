#ifndef COMPILER_AST_H
#define COMPILER_AST_H

typedef struct ASTNode ASTNode;

typedef enum NodeType {
    LITERAL,
    ID,
    OPER
} NodeType;

typedef struct Literal {
    int value;
} Literal;

typedef struct Id {
    char *name;
} Id;

typedef struct Oper {
    int type;
    ASTNode** ops;
} Oper;

struct ASTNode {
    NodeType type;

    union {
        Literal literal;
        Id id;
        Oper oper;
    };
};

#include <stdarg.h>

ASTNode* oper(int type, int nops, ...);
ASTNode* id(char *name);
ASTNode* literal(int value);
void free_AST(ASTNode*);

#endif //COMPILER_AST_H
