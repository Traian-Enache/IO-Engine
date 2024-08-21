#include "iosvc_def.h"

#include <time.h>

#include "heaputils.h"
#include "delay_heap_entry.h"

io_errcode iosvc_post(io_service *iosvc, io_handler hnd) {
    if (iosvc->status != RUNNING && iosvc->status != READY)
        return EIO_STOPPED;

    if (!hnd.callback)
        return EIO_INVARG;

    io_handler *pos = cbuf_push(&iosvc->sync_handlers);

    if (!pos)
        return EIO_NOMEM;

    *pos = hnd;
    return EIO_OK;
}

io_errcode iosvc_post_delay(io_service *iosvc, io_handler hnd,
                            io_errcode *status, int milliseconds)
{
    int64_t deadline = current_time() + milliseconds;

    if (!hnd.callback || milliseconds < 0)
        return EIO_INVARG;

    if (iosvc->status == STOPPING || iosvc->status == DONE)
        return EIO_STOPPED;

    delay_heap_entry *ent = dynarr_emplace_back(&iosvc->timed_handlers_heap);

    if (!ent)
        return EIO_NOMEM;

    *ent = (delay_heap_entry){
        .handler = hnd,
        .status = status,
        .deadline = deadline
    };

    heap_sift_up(&iosvc->timed_handlers_heap, ent,
                 delay_hp_ent_lt, delay_hp_ent_swp);

    return EIO_OK;
}
