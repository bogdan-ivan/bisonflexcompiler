#include "scope.h"
#include "registers.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

int DEBUG = 0;

static void err(const char *fmt, ...);

/* convention when searching for registers */
static Identifier *NOT_FOUND = NULL;

/* symbol table */
int sym[REGISTER_COUNT];
// env state
Environment env;

int *scope(char *name) {
    // char idstr[2] = { (char)(id + 'a'), '\0' };
    err("Scoping...\n");
    return track(name);
}

int scope_register_index(char *name) {
    err("Scoping..\n");
    return track_register_index(name);
}

int track_register_index(char *name) {
    Identifier *id;

    err("Tracking %s\n", name);

    id = find_identifier_recursive(name);

    if (found(id)) {
        err("Found @%d; value: %d.\n\n", id->index, sym[id->index]);
        return id->index;
    }

    printf("Err: Identifier ` %s ` not found.\n", name);
    exit(65);
}

int *track(char *name) {
    Identifier *id;

    err("Tracking: %s.\n", name);

    id = find_identifier_recursive(name);

    if (found(id)) {
        err("Found @%d; value: %d.\n\n", id->index, sym[id->index]);
        return addrof_register(id->index);
    }

    printf("Err: Identifier ` %s ` not found.\n", name);
    exit(65);
}

// register identifier
Identifier *declare_identifier(char *name) {
    Identifier *id;

    if (used_identifier(name)) {
        printf("Warn: Identifier ` %s ` already used.", name);
        return find_identifier(name);
        //exit(69); // unavailable resource
    }
    // sizeof(Header) + sizeof name;
    id = (Identifier *) malloc(sizeof(*id));
    // init
    id->name = _strdup(name); // Check if there's a need for this.
    id->index = next_available_register();
    // add
    HASH_ADD_KEYPTR(hh, env.identifiers, id->name, strlen(id->name), id);

    return id;
}

// search key value store
Identifier *find_identifier_recursive(char *name) {
    Identifier *id;

    id = find_identifier(name);
    Environment saved_env = env;

    /* Walk up the scope until id is found, or top level is reached. */
    while (!found(id) && next_env()) {
        env = *next_env();
        id = find_identifier(name);
    }
    env = saved_env; // restore env

    return id;
}

// find identifier in the current environment
Identifier *find_identifier(char *name) {
    Identifier *result;
    HASH_FIND_STR(env.identifiers, name, result);

    if (result) {
        return result;
    }
    return NOT_FOUND;
}

// status check
bool used_identifier(char *name) {
    return found(find_identifier(name));
}

bool used_identifier_recursive(char *name) {
    bool used = used_identifier(name);
    Environment saved_env = env;
    while (!used && next_env()) {
        env = *next_env();
        used = used_identifier(name);
    }
    env = saved_env;
    return used;
}

// register work
//int next_available_register() {
//  int i;
//  for (i = 0; inrange(i); ++i) {
//    if (!used_sym[i]) {
//      used_sym[i] = true;
//      return i;
//    }
//  }
//  printf("Unavailable memory, after %d iter.\n", i);
//  exit(69); // unavailable memory
//}

//void free_register(int index) {
//  if (inrange(index)) {
//    used_sym[index] = false;
//  }
//}

int *addrof_register(int index) {
    if (inrange(index)) {
        return &sym[index];
    }
    exit(65);
}

// utils
Environment *next_env() {
    return env.prev;
}

// checks result of find_identifier
bool found(Identifier *id) {
    return NOT_FOUND != id;
}

//// [is register index in range]
//bool inrange(int index) {
//  return index >= 0 && index < REGISTER_COUNT;
//}

void destruct_identifiers() {
    Identifier *current, *tmp;
    HASH_ITER(hh, env.identifiers, current, tmp) {
        free_register(current->index);
        free(current->name);
        HASH_DEL(env.identifiers, current);    /* delete it (identifiers advances to next) */
        free(current);
    }
}

// err printing
void err(const char *fmt, ...) {
    if (!DEBUG) return;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// cleanup
void destruct_environment() {
    destruct_identifiers();
    env.identifiers = NULL;
    env.prev = NULL;
}
