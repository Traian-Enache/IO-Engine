#include "iosvc_def.h"
#include "iosvc_dequeue.h"
#include "rbtree.h"

#include <poll.h>

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

    hnd.callback(hnd.ctx);

    // Check if any more ops are pending for current FD
    struct pollfd *pollvec = (struct pollfd *)dynarr_front(&iosvc->pollfds);

    if (pollvec[(*place)->pollfd_idx].events == 0)
        ioevt_cleanup_evt(iosvc, place, parent);

    return EIO_OK;
}
