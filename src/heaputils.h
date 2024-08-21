#ifndef IO_HEAPUTILS_H_
#define IO_HEAPUTILS_H_ 1

#include <time.h>

#include "dynarray.h"

void heap_sift_up_idx(dynarray *heap, size_t idx,
                      int (*lt)(const void *, const void *),
                      void (*swap)(void *, void *));

void heap_sift_down_idx(dynarray *heap, size_t idx,
                        int (*lt)(const void *, const void *),
                        void (*swap)(void *, void *));

inline static void heap_sift_up(dynarray *heap, void *elem,
                                int (*lt)(const void *, const void *),
                                void (*swap)(void *, void *)) {
    size_t idx = (size_t)((char *)elem - (char *)heap->data) / heap->obj_size;
    heap_sift_up_idx(heap, idx, lt, swap);
}

inline static void heap_sift_down(dynarray *heap, void *elem,
                                  int (*lt)(const void *, const void *),
                                  void (*swap)(void *, void *)) {
    size_t idx = (size_t)((char *)elem - (char *)heap->data) / heap->obj_size;
    heap_sift_down_idx(heap, idx, lt, swap);
}

inline static int64_t current_time() {
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    return curr_time.tv_sec * 1000L + curr_time.tv_nsec / 1000000L;
}

#endif // IO_HEAPUTILS_H_
