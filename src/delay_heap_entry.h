#ifndef IO_DELAY_HEAP_ENTRY_H_
#define IO_DELAY_HEAP_ENTRY_H_ 1

#include <stdint.h>

#include "iotypes.h"

typedef struct delay_heap_entry {
    int64_t deadline;
    io_handler handler;
    io_errcode *status;
} delay_heap_entry;

int delay_hp_ent_lt(const void *ent1, const void *ent2);

void delay_hp_ent_swp(void *ent1, void *ent2);

#endif // IO_DELAY_HEAP_ENTRY_H_
