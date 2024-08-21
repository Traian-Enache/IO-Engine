#include "iosvc_def.h"
#include "iosvc_dequeue.h"
#include "rbtree.h"

#include <poll.h>
#include <stdio.h>

io_errcode iosvc_cancel(io_service *iosvc, io_event event) {
    if (iosvc->status != RUNNING)
        return EIO_INVARG;

    rb_place place;
    rb_node *parent;

    place = rb_probe(&iosvc->async_handlers, &parent, event.fd);
    if (!(*place))
        return EIO_NOENTRY;

    event_data *evt_data = get_evt_data(*place, event.wait_type);

    if (!evt_data || !evt_data->handler.callback)
        return EIO_NOENTRY;

    io_handler hnd = iosvc_dequeue(iosvc, *place, event.wait_type,
                                   0, EIO_CANCELLED);

    (void)hnd; // hnd.callback(hnd.ctx); TODO

    static const char *wait_types_names[] = {
        [WAIT_READ] = "WAIT_READ",
        [WAIT_WRITE] = "WAIT_WRITE",
        [WAIT_EXCEPTION] = "WAIT_EXCEPTION"
    };
    fprintf(stderr, "[LOG] Calling %p with arg %p upon cancellation of event{fd=%d, wt=%s}\n",
        (void*)(uintptr_t)hnd.callback, hnd.ctx, event.fd, wait_types_names[event.wait_type]);

    // Check if any more ops are pending for current FD
    struct pollfd *pollvec = (struct pollfd *)dynarr_front(&iosvc->pollfds);

    if (pollvec[(*place)->pollfd_idx].events == 0)
        ioevt_cleanup_evt(iosvc, place, parent);

    return EIO_OK;
}
