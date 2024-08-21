#include "async_heap_entry.h"

int async_hp_ent_lt(const void *ent1, const void *ent2) {
    return ((async_heap_entry const *)ent1)->deadline <
           ((async_heap_entry const *)ent2)->deadline;
}

void async_hp_ent_swp(void *ent1, void *ent2) {
    async_heap_entry *elem1 = (async_heap_entry *)ent1;
    async_heap_entry *elem2 = (async_heap_entry *)ent2;

    async_heap_entry tmp = *elem1;
    *elem1 = *elem2;
    *elem2 = tmp;

    event_data *evt1 = get_evt_data(elem1->io_op_data, elem1->event_type);
    event_data *evt2 = get_evt_data(elem2->io_op_data, elem2->event_type);

    ptrdiff_t tmp_heap_idx = evt1->ddl_heap_idx;
    evt1->ddl_heap_idx = evt2->ddl_heap_idx;
    evt2->ddl_heap_idx = tmp_heap_idx;
}
