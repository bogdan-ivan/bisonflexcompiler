#ifndef __req_hh
#define __req_hh

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* stdout colored output. */
void print_red(const char *);
void print_green(const char *);

#define EXPAND__MACRO( x ) x
#define GET__one_two(_1, _2, NAME, ...) NAME

/* TODO: req with only one arg: parse the *code* using a function (if_cpp constexpr fn). */
#define req(...) EXPAND__MACRO(GET__one_two(__VA_ARGS__, req__two, req__one)(__VA_ARGS__))

#define req__one(test__condition)                                       \
    require((test__condition) ? (1) : (0), #test__condition, __FUNCTION__, __LINE__)

#define req__two(test__condition, err__msg)                             \
    require_with_errmsg((test__condition) ? (1) : (0), #test__condition, __FUNCTION__, __LINE__, err__msg)

static inline void fatal_print_begin(const char* assertion)
{
    printf("\n[ ");
    print_red("Err");
    printf(" ] False assertion: ");
    printf("%s", assertion);
}

static inline void fatal_print_end(const char* calling_function, int line)
{
    printf("\t@%s() <%d>\n", calling_function, line);
    fflush(stdout);
    exit(-1);
}

static inline bool require(bool clause, const char* assertion, const char* calling_function, int line)
{
    if (!clause) {
        fatal_print_begin(assertion);
        putchar('.');
        fatal_print_end(calling_function, line);
    }
    return true;
}

static inline bool require_with_errmsg(bool clause, const char* assertion, const char* calling_function, int line, const char* err_msg)
{
    if (!clause) {
        fatal_print_begin(assertion);
        printf(".  // %s", err_msg);
        fatal_print_end(calling_function, line);
    }
    return true;
}

#endif
