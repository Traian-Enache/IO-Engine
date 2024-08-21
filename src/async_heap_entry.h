#ifndef IO_ASYNC_HEAP_ENTRY_H_
#define IO_ASYNC_HEAP_ENTRY_H_ 1

#include <stdint.h>

#include "rbnode.h"

typedef struct async_heap_entry {
    int64_t deadline;
    int *remaining;
    rb_node *io_op_data;
    io_wait_type event_type;
} async_heap_entry;

int async_hp_ent_lt(const void *ent1, const void *ent2);

void async_hp_ent_swp(void *ent1, void *ent2);

#endif // IO_ASYNC_HEAP_ENTRY_H_
