#ifndef IO_RBNODE_H_
#define IO_RBNODE_H_ 1

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "iotypes.h"

typedef struct event_data {
    io_handler handler;
    io_errcode *status;
    ptrdiff_t ddl_heap_idx;
} event_data;

typedef struct rb_node {
    uintptr_t meta; // Parent ptr + Color (R/B) in LSB

    union {
        struct {
            struct rb_node *left;
            struct rb_node *right;
        };
        struct rb_node *child[2];
    };

    int fd;
    enum {
        EMPTY = 0,
        READ,
        WRITE
    } main_ev_type;

    size_t pollfd_idx;

    // Store one event inline with the node, to avoid allocations, as there
    // usually is only one event issued per FD.
    // Said event can be either read or write, exception events are stored
    // indirectly as they are very uncommonly used
    event_data main_event;
    event_data *aux_event;
    event_data *ex_event;
} rb_node;

inline static rb_node *make_rbnode(int fd) {
    rb_node *node = (rb_node *)calloc(1, sizeof(*node));

    if (node) {
        node->fd = fd;
        node->meta = 1UL; // Set as red
    }

    return node;
}

inline static event_data *get_evt_data(rb_node *node, io_wait_type ev_type) {
    if (ev_type == WAIT_EXCEPTION)
        return node->ex_event;

    if (node->main_ev_type == EMPTY)
        return NULL;

    if ((node->main_ev_type == READ && ev_type == WAIT_READ) ||
        (node->main_ev_type == WRITE && ev_type == WAIT_WRITE))
        return &node->main_event;

    return node->aux_event;
}

#endif // IO_RBNODE_H_
