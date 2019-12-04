#include "buffer.h"
#include "req.hh"


void buf_pop_typed(char *buf) {
    req(buf_size_typed(buf) != 0, "Cannot pop empty buffer.");
    buf_hdr(buf)->size--;
}


char* buf_resize_typed(char *old_buf, size_t new_size, size_t elem_size) {
    if (0 == new_size) return buf_free_typed(old_buf), NULL;
    return buf_realloc_typed(old_buf, new_size, elem_size);;
}

char* buf_fit_typed(char *buf, size_t elem_size) {
    if (buf_fits_typed(buf)) return buf;
    size_t new_size = MAX(buf_size_typed(buf) * 2 + 1, MIN_CAP_THRESHOLD);

    return buf_resize_typed(buf, new_size, elem_size);
}

// allocate buffer
char* buf_make_typed(size_t size, size_t elem_size) {
    BufHdr* new_buf = (BufHdr*) malloc(sizeof(BufHdr) + size * elem_size);
    req(NULL != new_buf, "Malloc failed.");
    new_buf->size = 0;
    new_buf->cap = size;
    return new_buf->buf;
}

// reallocate buffer
char* buf_realloc_typed(char *buf, size_t size, size_t elem_size) {
    BufHdr* new_buf;
    if (NULL == buf) {
        return buf_make_typed(size, elem_size);
    }
    new_buf = (BufHdr*) realloc(buf_hdr(buf), sizeof(BufHdr) + size * elem_size);
    req(NULL != new_buf, "Realloc failed.");
    new_buf->cap = size;

    // handle shrinking
    if (new_buf->size > new_buf->cap) {
        new_buf->size = new_buf->cap;
    }
    return new_buf->buf;
}

void buf_free_transitive_typed(char **buf, size_t elem_size) {
    if (NULL == buf) return;
    // for i=0...buf_size
    buf_range(i, buf) {
        // Free non-NULL elements.
        if (NULL != buf[i * elem_size])
            free(buf[i * elem_size]);
    }
    buf_free_typed((char*)buf);
}

void buf_free_typed(char *buf) {
    if (NULL != buf) {
        free(buf_hdr(buf));
    }
}

size_t buf_cap_typed(char *buf) {
    if (NULL == buf) return 0;
    return buf_hdr(buf)->cap;
}

size_t buf_size_typed(char *buf) {
    if (NULL == buf) return 0;
    return buf_hdr(buf)->size;
}

bool buf_fits_typed(char *buf) {
    return buf_cap_typed(buf) - buf_size_typed(buf) > 0;
}

BufHdr* buf_hdr(char *buf) {
    if (NULL == buf) return NULL;
    return (BufHdr*)(buf - sizeof(BufHdr));
}


