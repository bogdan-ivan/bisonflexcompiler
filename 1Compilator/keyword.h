#ifndef  __keyword_h
#define __keyword_h

#include "uthash.h"
#include "AST.h"
#include "y.tab.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

#define KEYWORD_MAXLEN 10

// use yacc tokens
typedef enum yytokentype keyword_t;

typedef struct Keyword Keyword;
struct Keyword {
    keyword_t type;
    char name[KEYWORD_MAXLEN];
    UT_hash_handle hh;
};

void add_keyword(Keyword *);

keyword_t find_keyword_or(char *name, keyword_t or);

void init_keywords();

void destruct_keywords();

#endif /* end of include guard: __keyword_h */
