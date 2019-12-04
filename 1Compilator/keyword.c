#include "keyword.h"

// keywords -> TOKEN_TYPE
Keyword *keywords;

// map keyword
void add_keyword(Keyword *keyword) {
    if (!keyword || KEYWORD_MAXLEN <= strlen(keyword->name)) {
        exit(65);
    }
    HASH_ADD_STR(keywords, name, keyword);
}

keyword_t find_keyword_or(char *name, keyword_t or) {
    Keyword *result;
    HASH_FIND_STR(keywords, name, result);

    if (result) {
        return result->type;
    }
    return or;
}

Keyword keywords_array[] = {
        {WHILE, "while"},
        {FOR,   "for"},
        {IF,    "if"},
        {ELSE,  "else"},
        {PRINT, "print"},
        {OR,    "or"},
        {AND,   "and"},
        {VAR,   "var"}
};
// { "break", BREAK }

void init_keywords() {
    keywords = NULL;
    for (int i = 0; i < NELEMS(keywords_array); ++i) {
        add_keyword(&keywords_array[i]);
    }
}

void destruct_keywords() {
    Keyword *current, *tmp;
    HASH_ITER(hh, keywords, current, tmp) {
        HASH_DEL(keywords, current);    /* delete it (keywords advances to next) */
        //free(current); keywords are on the stack
    }
}
