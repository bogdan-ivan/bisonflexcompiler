#ifndef COMPILER_BUFFER_H
#define COMPILER_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN_CAP_THRESHOLD 5

typedef struct BufHdr BufHdr;

struct BufHdr {
    size_t size;
    size_t cap;
    char buf[];
};

BufHdr* buf_hdr(char *buf);
size_t buf_size_typed(char *buf);
size_t buf_cap_typed(char *buf);
bool buf_fits_typed(char *buf);
char* buf_fit_typed(char *buf, size_t elem_size);

// memory
char* buf_make_typed(size_t size, size_t elem_size);
char* buf_realloc_typed(char *buf, size_t size, size_t elem_size);
char* buf_resize_typed(char *old_buf, size_t new_size, size_t elem_size);
void buf_free_typed(char *buf);
void buf_free_transitive_typed(char **buf, size_t elem_size);
void buf_pop_typed(char *buf);

#define buf_resize(buf, new_size) \
    buf = (void*)buf_resize_typed((char*) buf, new_size, sizeof(*buf))

#define buf_push(buf, val) \
    buf = (void*)buf_fit_typed((char *)buf, sizeof(*buf)), \
    buf[buf_hdr((char *)buf)->size++] = val

#define buf_pop(buf) \
    buf_pop_typed((char*)buf)

#define buf_size(buf) buf_size_typed((char*)buf)
#define buf_cap(buf) buf_cap_typed((char*)buf)

#define buf_free(buf) \
    buf_free_typed((char*)buf)

#define buf_free_transitive(buf) \
        (void)sizeof(**buf), \
        buf_free_transitive_typed((char**)buf, sizeof(*buf))

#define buf_range(index_name, buf) \
    for (size_t index_name = 0; index_name < buf_size(buf); ++index_name)

//void buf_cpy(char* new_buf, char* old_buf); TODO

#endif //COMPILER_BUFFER_H
