#include "iosvc_dequeue.h"
#include "async_heap_entry.h"
#include "heaputils.h"

#include <poll.h>

io_handler iosvc_dequeue(io_service *iosvc, rb_node *fd_node,
                         io_wait_type event_type, int64_t now,
                         io_errcode status)
{
    event_data *evt_data = get_evt_data(fd_node, event_type);
    io_handler hnd = evt_data->handler;

    ptrdiff_t heap_idx = evt_data->ddl_heap_idx;

    // Signal remaining time for operation and pop from events heap
    if (heap_idx >= 0) {
        dynarray *heap = &iosvc->timed_events_heap;
        async_heap_entry *heap_ent =
            &((async_heap_entry *)heap->data)[heap_idx];

        int remaining = heap_ent->deadline > now ?
            (int)(heap_ent->deadline - now) :
            0;
        *heap_ent->remaining = remaining;

        // Pop heap entry

        heap_ent->deadline = -1; // Value lower than all entries, to move to top
        heap_sift_up_idx(heap, (size_t)heap_idx,
                         async_hp_ent_lt, async_hp_ent_swp);

        async_hp_ent_swp(dynarr_front(heap), dynarr_back(heap));
        dynarr_pop_back(heap);

        if (!dynarr_empty(heap))
            heap_sift_down_idx(heap, 0, async_hp_ent_lt, async_hp_ent_swp);
    }

    // Vacate event data storage
    evt_data->handler.callback = NULL;

    if (evt_data->status)
        *evt_data->status = status;

    if (!fd_node->main_event.handler.callback &&
        !(fd_node->aux_event && fd_node->aux_event->handler.callback))
        fd_node->main_ev_type = EMPTY;

    // Unset pollfd event flags
    struct pollfd *pollent =
        &((struct pollfd *)iosvc->pollfds.data)[fd_node->pollfd_idx];

    static short const evt_masks[] = {
        [WAIT_READ] = (POLLIN | POLLHUP | POLLRDNORM | POLLRDBAND),
        [WAIT_WRITE] = (POLLOUT | POLLWRNORM | POLLWRBAND),
        [WAIT_EXCEPTION] = POLLPRI
    };

    pollent->events &= ~evt_masks[event_type];

    if ((pollent->events & (evt_masks[WAIT_READ] | evt_masks[WAIT_WRITE])) == 0)
        pollent->events &= ~POLLERR;

    return hnd;
}

void ioevt_cleanup_evt(io_service *iosvc, rb_place place, rb_node *parent) {
    rb_node *to_remove = rb_extract(place, parent, &iosvc->async_handlers);
    
    size_t pollfd_idx = to_remove->pollfd_idx;
    free(to_remove->aux_event);
    free(to_remove->ex_event);
    free(to_remove);

    // If removed node's pollfd is not the last position in pollvec,
    // move last pollfd to current index, and update its node to reflect
    // the new index

    struct pollfd *pollvec = (struct pollfd *)iosvc->pollfds.data;
    pollvec[pollfd_idx] = pollvec[iosvc->pollfds.nelems - 1];

    dynarr_pop_back(&iosvc->pollfds);

    if (pollfd_idx < dynarr_size(&iosvc->pollfds)) {
        place = rb_probe(&iosvc->async_handlers, &parent, pollvec[pollfd_idx].fd);
        (*place)->pollfd_idx = pollfd_idx;
    }
}
