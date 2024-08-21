#ifndef IO_DYNARRAY_H_
#define IO_DYNARRAY_H_ 1

#include <stddef.h>
#include <stdlib.h>

typedef struct dynarray {
    void *data;
    size_t nelems;
    size_t capacity;
    size_t obj_size;
} dynarray;

void dynarr_init(dynarray *const dar, size_t obj_size);

void dynarr_delete(dynarray *const dar);

void *dynarr_emplace_back(dynarray *const dar);

inline static void dynarr_pop_back(dynarray *const dar) {
    --dar->nelems;
}

inline static size_t dynarr_size(dynarray *const dar) {
    return dar->nelems;
}

inline static int dynarr_empty(dynarray *const dar) {
    return dar->nelems == 0;
}

inline static void *dynarr_at(dynarray *const dar, size_t idx) {
    return (void *)((char *)dar->data + idx * dar->obj_size);
}

inline static void *dynarr_front(dynarray *const dar) {
    return dar->data;
}

inline static void *dynarr_back(dynarray *dar) {
    return dynarr_at(dar, dar->nelems - 1);
}

inline static void *dynarr_end(dynarray *dar) {
    return dynarr_at(dar, dar->nelems);
}

inline static void dynarr_clear(dynarray *dar) {
    dar->nelems = 0;
}

#endif // IO_DYNARRAY_H_
