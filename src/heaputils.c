#include "heaputils.h"

inline static size_t up(size_t idx) {
    return (idx - 1) / 2;
}

inline static size_t left(size_t idx) {
    return 2 * idx + 1;
}

// To get right just use left + 1

void heap_sift_up_idx(dynarray *heap, size_t idx,
                      int (*lt)(const void *, const void *),
                      void (*swap)(void *, void *)) {
    void *curr_elem = dynarr_at(heap, idx);

    while (idx != 0) {
        size_t parent = up(idx);
        void *parent_elem = dynarr_at(heap, parent);

        if (lt(curr_elem, parent_elem)) {
            swap(parent_elem, curr_elem);
            curr_elem = parent_elem;
            idx = parent;
        } else {
            break;
        }
    }
}

void heap_sift_down_idx(dynarray *heap, size_t idx,
                        int (*lt)(const void *, const void *),
                        void (*swap)(void *, void *)) {
    void *curr_elem = dynarr_at(heap, idx);

    while (1) {
        size_t min_idx = idx;
        size_t left_idx = left(idx);
        void *min_elem = curr_elem;

        if (left_idx >= heap->nelems)
            break;

        void *left_elem = dynarr_at(heap, left_idx);

        if (lt(left_elem, min_elem)) {
            min_idx = left_idx;
            min_elem = left_elem;
        }

        size_t right_idx = left_idx + 1;

        if (right_idx < heap->nelems) {
            void *right_elem = (char *)left_elem + heap->obj_size;

            if (lt(right_elem, min_elem)) {
                min_idx = right_idx;
                min_elem = right_elem;
            }
        }

        if (min_idx != idx) {
            swap(min_elem, curr_elem);
            idx = min_idx;
            curr_elem = min_elem;
        } else {
            break;
        }
    }
}
