#ifndef IO_CBUFFER_H_
#define IO_CBUFFER_H_ 1

#include <stddef.h>

typedef struct cbuffer {
    char *data;
    char *data_limit;
    size_t nelems;
    size_t obj_size;
    char *begin;
    char *end;
} cbuffer;

void cbuf_init(cbuffer *cbuf, size_t obj_size);

void cbuf_delete(cbuffer *cbuf);

void *cbuf_push(cbuffer *cbuf);

inline static size_t cbuf_size(cbuffer const *cbuf) {
    return cbuf->nelems;
}

inline static int cbuf_empty(cbuffer const *cbuf) {
    return cbuf->nelems == 0;
}

inline static void *cbuf_front(cbuffer *cbuf) {
    return cbuf->begin;
}

inline static void cbuf_pop(cbuffer *cbuf) {
    --cbuf->nelems;
    cbuf->begin += cbuf->obj_size;
    if (cbuf->begin == cbuf->data_limit)
        cbuf->begin = cbuf->data;
}

inline static void cbuf_clear(cbuffer *cbuf) {
    cbuf->nelems = 0;
    cbuf->begin = cbuf->end = cbuf->data;
}

#endif // IO_CBUFFER_H_
