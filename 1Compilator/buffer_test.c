#include "buffer.h"
#include "req.hh"

int main() {
    int *arr = NULL;

    req(buf_cap(arr) == 0, "cap");
    req(buf_size(arr) == 0, "size");

    buf_push(arr, 2);

    req(buf_cap(arr) > 0, "cap>0");
    req(buf_size(arr) == 1, "size=1");
    req(arr[0] == 2, "fail0");

    buf_push(arr, 2341);
    req(arr[1] == 2341, "fail1");
    buf_push(arr, 242);
    req(arr[2] == 242, "fail2");

    size_t old_size = buf_size(arr);
    buf_resize(arr, 10);
    req(buf_cap(arr) == 10, "cap==0");
    req(buf_size(arr) == old_size, "size=old_size");

    old_size = buf_size(arr);
    buf_resize(arr, 2);
    req(buf_cap(arr) == 2, "cap==0");
    req(buf_size(arr) != old_size, "size!=old_size");
    req(buf_size(arr) == buf_cap(arr), "size=cap");

    buf_pop(arr);

    buf_free(arr);

    int **ptrs = NULL;
    int* x_ptr = malloc(sizeof(*x_ptr));
    buf_push(ptrs, x_ptr);

    buf_free_transitive(ptrs);
    return 0;
}