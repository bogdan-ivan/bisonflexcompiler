#ifndef __scope_h
#define __scope_h

#include "uthash.h"

#include <stdbool.h>

// forward decl
typedef struct Environment Environment;
typedef struct Identifier Identifier;

struct Environment {
    Identifier *identifiers;
    Environment *prev;
};

struct Identifier {
    char *name;
    int index;
    UT_hash_handle hh;
};

// current environment
extern Environment env;

Environment *next_env(); // walk up the scope
void destruct_environment(); // cleanup
int *scope(char *id);
int *track(char *name);

int scope_register_index(char *name);
int track_register_index(char *name);

// identifiers
Identifier *declare_identifier(char *name);

Identifier *find_identifier(char *name);

Identifier *find_identifier_recursive(char *name);

bool found(Identifier *);

bool used_identifier(char *name);

bool used_identifier_recursive(char *name);

// registers
int *addrof_register(int index);

void free_register(int index);
// utils
bool inrange(int index);

#endif /* end of include guard: __scope_h */
