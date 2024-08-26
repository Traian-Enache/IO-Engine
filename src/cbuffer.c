#include "cbuffer.h"

#include <memory.h>
#include <stdlib.h>

inline static size_t mx(size_t a, size_t b) {
    return a < b ? b : a;
}

void cbuf_init(cbuffer *cbuf, size_t obj_size) {
    *cbuf = (cbuffer){.data = NULL,
                      .data_limit = NULL,
                      .nelems = 0,
                      .obj_size = obj_size,
                      .begin = NULL,
                      .end = NULL};
}

void cbuf_delete(cbuffer *cbuf) {
    free(cbuf->data);
}

void *cbuf_push(cbuffer *cbuf) {
    if (!cbuf->data || ((cbuf->begin == cbuf->end) && cbuf->nelems != 0)) {
        // resize the buffer
        size_t new_capacity =
            mx(cbuf->obj_size, 2u * (size_t)(cbuf->data_limit - cbuf->data));
        char *new_data = (char *)malloc(new_capacity);

        if (!new_data)
            return NULL;

        size_t fst_copy_size = (size_t)(cbuf->data_limit - cbuf->begin);
        size_t scd_copy_size = (size_t)(cbuf->end - cbuf->data);
        memmove(new_data, cbuf->begin, fst_copy_size);
        memmove(new_data + fst_copy_size, cbuf->data, scd_copy_size);

        free(cbuf->data);

        cbuf->data = new_data;
        cbuf->data_limit = new_data + new_capacity;
        cbuf->begin = new_data;
        cbuf->end = new_data + fst_copy_size + scd_copy_size;
    }

    void *retval = cbuf->end;
    ++cbuf->nelems;

    cbuf->end += cbuf->obj_size;
    if (cbuf->end == cbuf->data_limit)
        cbuf->end = cbuf->data;

    return retval;
}
