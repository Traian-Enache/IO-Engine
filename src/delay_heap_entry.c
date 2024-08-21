#include "delay_heap_entry.h"

int delay_hp_ent_lt(const void *ent1, const void *ent2) {
    return ((delay_heap_entry const *)ent1)->deadline <
           ((delay_heap_entry const *)ent2)->deadline;
}

void delay_hp_ent_swp(void *ent1, void *ent2) {
    delay_heap_entry tmp = *(delay_heap_entry *)ent1;
    *(delay_heap_entry *)ent1 = *(delay_heap_entry *)ent2;
    *(delay_heap_entry *)ent2 = tmp;
}
