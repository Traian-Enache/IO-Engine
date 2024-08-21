#include "dynarray.h"

#define DYNARR_MIN_CAPACITY 1

inline static size_t mx(size_t a, size_t b) {
    return a < b ? b : a;
}

void *dynarr_emplace_back(dynarray *const dar) {
    if (dar->nelems == dar->capacity) {
        size_t new_capacity = mx(2 * dar->capacity, DYNARR_MIN_CAPACITY);
        size_t new_size = dar->obj_size * new_capacity;

        void *new_data = realloc(dar->data, new_size);

        if (!new_data)
            return NULL;

        dar->data = new_data;
        dar->capacity = new_capacity;
    }

    void *retval = dynarr_end(dar);
    ++dar->nelems;

    return retval;
}

void dynarr_init(dynarray *const dar, size_t obj_size) {
    *dar = (dynarray){
        .data = NULL, .capacity = 0, .nelems = 0, .obj_size = obj_size};
}

void dynarr_delete(dynarray *const dar) {
    free(dar->data);
}
