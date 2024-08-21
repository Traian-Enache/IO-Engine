#include "io_service.h"
#include "iosvc_def.h"

#include <poll.h>
#include <errno.h>
#include <time.h>

#include "async_heap_entry.h"
#include "delay_heap_entry.h"
#include "heaputils.h"
#include "iosvc_dequeue.h"

io_errcode iosvc_stop(io_service *iosvc) {
    switch (iosvc->status) {
    case RUNNING:
        iosvc->status = STOPPING;
        return EIO_OK;

    case STOPPING:
        return EIO_INPROGRESS;

    default:
        return EIO_NOENTRY;
    }
}

io_errcode iosvc_reset(io_service *iosvc) {
    switch (iosvc->status) {
    case RUNNING:
    case STOPPING:
        return EIO_INPROGRESS;

    default:
        iosvc->status = READY;
        return EIO_OK;
    }
}

static void run_sync_handlers(io_service *iosvc) {
    cbuffer *handlers = &iosvc->sync_handlers;

    while (!cbuf_empty(handlers)) {
        io_handler *curr_hnd = cbuf_front(handlers);
        curr_hnd->callback(curr_hnd->ctx);
        cbuf_pop(handlers);
    }
}

/**
 * @brief Stop an async event's handler. Sets status to `EIO_STOPPED` if 
 * provided, sets remaining time (if this is a timeout event)
 * and calls the handler's callback
 * 
 * @param evt event to be stopped
 * @param now current time
 * @param hp deadlines heap
 */
static void stop_handler(event_data *evt, int64_t now, async_heap_entry *hp) {
    if (evt->status)
        *evt->status = EIO_STOPPED;

    if (evt->ddl_heap_idx >= 0) {
        async_heap_entry *heap_ent = &hp[evt->ddl_heap_idx];

        int64_t remaining = heap_ent->deadline - now;
        *heap_ent->remaining = (remaining > 0L ? (int)remaining : 0L);
    }

    evt->handler.callback(evt->handler.ctx);
}

/**
 * @brief Stop all asynchronous operations contained in provided node.
 * Recursively call for child nodes
 * 
 * @param node node containing handlers and associated data
 * @param now current time
 * @param hp deadline heap
 */
static void stop_async_ops(rb_node *node, int64_t now, async_heap_entry *hp) {
    if (!node)
        return;

    if (node->main_event.handler.callback)
        stop_handler(&node->main_event, now, hp);

    if (node->aux_event && node->aux_event->handler.callback)
        stop_handler(node->aux_event, now, hp);

    if (node->ex_event && node->ex_event->handler.callback)
        stop_handler(node->ex_event, now, hp);

    free(node->aux_event);
    free(node->ex_event);

    stop_async_ops(node->left, now, hp);
    stop_async_ops(node->right, now, hp);

    free(node);
}

/**
 * @brief Stop the io_service
 * 
 * @param iosvc service to stop
 */
static void iosvc_prep_stop(io_service *iosvc) {
    // Run timed non-event handlers
    delay_heap_entry *curr_ent = 
        (delay_heap_entry *)dynarr_front(&iosvc->timed_handlers_heap);

    for (size_t i = 0; i < iosvc->timed_handlers_heap.nelems; ++i) {
        if (curr_ent->status)
            *curr_ent->status = EIO_STOPPED;

        io_handler *handler = &curr_ent->handler;
        handler->callback(handler->ctx);

        // Pointer is never invalidated as new callbacks cant be registered
        // if service is stopped
        ++curr_ent;
    }

    dynarr_clear(&iosvc->timed_handlers_heap);

    // Stop all asynchronous operations
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);

    int64_t now = curr_time.tv_nsec / 1000000L + curr_time.tv_sec * 1000L;

    stop_async_ops(iosvc->async_handlers, now,
                   (async_heap_entry *)dynarr_front(&iosvc->timed_events_heap));
    dynarr_clear(&iosvc->timed_events_heap);
    dynarr_clear(&iosvc->pollfds);
    iosvc->async_handlers = NULL;
}

/**
 * @brief Fetch next deadline
 * 
 * @param iosvc service to fetch next deadline from
 * @param is_event out parameter specifying if the next deadline is from
 * an async event or from a timed, non-event handler
 * @return next deadline, in milliseconds 
 */
static int64_t next_deadline(io_service *iosvc, int *is_event) {
    int64_t deadline = -1;

    if (!dynarr_empty(&iosvc->timed_handlers_heap)) {
        delay_heap_entry *top =
            (delay_heap_entry *)dynarr_front(&iosvc->timed_handlers_heap);

        deadline = top->deadline;
        *is_event = 0;
    }

    if (!dynarr_empty(&iosvc->timed_events_heap)) {
        async_heap_entry *top =
            (async_heap_entry *)dynarr_front(&iosvc->timed_events_heap);
        
        if (deadline == -1 || top->deadline < deadline) {
            deadline = top->deadline;
            *is_event = 1;
        }
    }

    return deadline;
}

int is_top_event_ready(io_service *iosvc) {
    if (dynarr_empty(&iosvc->timed_events_heap))
        return 0;

    async_heap_entry *top_event =
        (async_heap_entry *)dynarr_front(&iosvc->timed_events_heap);
    
    size_t poll_idx = top_event->io_op_data->pollfd_idx;

    static short const evt_masks[] = {
        [WAIT_READ] = (POLLIN | POLLHUP | POLLRDNORM | POLLRDBAND | POLLERR),
        [WAIT_WRITE] = (POLLOUT | POLLWRNORM | POLLWRBAND | POLLERR),
        [WAIT_EXCEPTION] = POLLPRI
    };

    struct pollfd *pollvec = (struct pollfd *)iosvc->pollfds.data;

    return (pollvec[poll_idx].revents & evt_masks[top_event->event_type]) != 0;
}

/**
 * @brief Remove the next timed event from the pending set and call its
 * handler
 * 
 * @param iosvc service to remove the event from
 * @param is_event type of operation (1 if fd-based event, 0 if delayed handler)
 * @param now current time
 */
void timeout_first(io_service *iosvc, int is_event, int64_t now) {
    if (is_event) {
        async_heap_entry *top_event =
            (async_heap_entry *)dynarr_front(&iosvc->timed_events_heap);
        
        rb_node *node = top_event->io_op_data;

        int *remaining_ptr = top_event->remaining;
        io_handler hnd = iosvc_dequeue(iosvc, node, top_event->event_type,
                                       now, EIO_TIMEOUT);
        *remaining_ptr = 0;
        hnd.callback(hnd.ctx);

        rb_node *parent;
        rb_place ref_to_this = rb_ref_to_this(node, &iosvc->async_handlers,
                                              &parent);

        struct pollfd *pollvec = (struct pollfd *)dynarr_front(&iosvc->pollfds);

        if (pollvec[node->pollfd_idx].events == 0)
            ioevt_cleanup_evt(iosvc, ref_to_this, parent);
    } else {
        delay_heap_entry *top_callback =
            (delay_heap_entry *)dynarr_front(&iosvc->timed_handlers_heap);
        
        if (top_callback->status)
            *top_callback->status = EIO_OK;
        top_callback->handler.callback(top_callback->handler.ctx);

        // Handler might have resized vector, get beginning again
        top_callback =
            (delay_heap_entry *)dynarr_front(&iosvc->timed_handlers_heap);
        *top_callback = top_callback[iosvc->timed_handlers_heap.nelems - 1];
        dynarr_pop_back(&iosvc->timed_handlers_heap);

        if (!dynarr_empty(&iosvc->timed_handlers_heap))
            heap_sift_down_idx(&iosvc->timed_handlers_heap, 0,
                               delay_hp_ent_lt, delay_hp_ent_swp);
    }
}

void dispatch_ready_events(io_service *iosvc, int64_t now) {
#define PVEC ((struct pollfd *)iosvc->pollfds.data)

    for (size_t i = 0; i < dynarr_size(&iosvc->pollfds);) {;
        rb_node *parent;
        rb_place place = rb_probe(&iosvc->async_handlers, &parent, 
                                    PVEC[i].fd);

        // Check if FD was invalid
        if (PVEC[i].revents & POLLNVAL) {
            static const short pfd_flags[] = {
                POLLIN, POLLOUT, POLLPRI
            };
            static const io_wait_type wts[] = {
                WAIT_READ, WAIT_WRITE, WAIT_EXCEPTION
            };
            
            // Call all handlers with EIO_INVARG
            for (size_t ev_idx = 0; ev_idx < 3; ++ev_idx)
                if (PVEC[i].events & pfd_flags[ev_idx]) {
                    io_handler hnd = iosvc_dequeue(iosvc, *place, wts[ev_idx],
                                                   now, EIO_INVARG);
                    hnd.callback(hnd.ctx);
                }
            
            PVEC[i].revents = 0;
            if (PVEC[i].events == 0)
                ioevt_cleanup_evt(iosvc, place, parent);
            continue;
        }

        int has_priority = PVEC[i].revents & POLLPRI;
        int has_error = PVEC[i].revents & POLLERR;
        int has_read = PVEC[i].revents & (POLLIN | POLLHUP | POLLRDBAND | POLLRDNORM);
        int has_write = PVEC[i].revents & (POLLOUT | POLLWRNORM | POLLWRBAND);

        if (has_priority) {
            io_handler hnd = iosvc_dequeue(iosvc, *place, WAIT_EXCEPTION, now, EIO_OK);
            hnd.callback(hnd.ctx);
        }
        if ((!has_read && !has_write && has_error &&
            (PVEC[i].events & ~POLLPRI) == POLLERR) || has_read) {
            // Either got read, or read and write pending but
            // got only error. Call read by convention
            io_handler hnd = iosvc_dequeue(iosvc, *place, WAIT_READ, now, EIO_OK);
            hnd.callback(hnd.ctx);
        }
        if (has_write) {
            io_handler hnd = iosvc_dequeue(iosvc, *place, WAIT_WRITE, now, EIO_OK);
            hnd.callback(hnd.ctx);
        }

        if (PVEC[i].events == 0) {
            ioevt_cleanup_evt(iosvc, place, parent);
            continue;
        }

        ++i;
    }

#undef PVEC
}

io_errcode iosvc_run(io_service *iosvc) {
    if (iosvc->status != READY)
        return (iosvc->status == DONE) ? EIO_INVARG : EIO_INPROGRESS;

    iosvc->status = RUNNING;

    while (!dynarr_empty(&iosvc->pollfds) ||
           !dynarr_empty(&iosvc->timed_handlers_heap) ||
           !cbuf_empty(&iosvc->sync_handlers)) {
        run_sync_handlers(iosvc);

        if (iosvc->status == STOPPING) {
            iosvc_prep_stop(iosvc);
            break;
        }

        if (dynarr_empty(&iosvc->pollfds) &&
            dynarr_empty(&iosvc->timed_handlers_heap))
            continue;

        int is_event;
        int64_t deadline = next_deadline(iosvc, &is_event);
        int64_t poll_time = current_time();
        int delay = (deadline == -1) ? -1 :
                    (deadline > poll_time) ? (int)(deadline - poll_time) :
                    0;

        int ready_fds = poll((struct pollfd *)dynarr_front(&iosvc->pollfds),
                                dynarr_size(&iosvc->pollfds),
                                delay);
        
        int64_t completion_time = current_time();
        int64_t elapsed = completion_time - poll_time;

        if (ready_fds < 0 && errno != EINTR) {
            iosvc->status = STOPPING;
            continue;
        } else if (ready_fds == 0) {
            timeout_first(iosvc, is_event, completion_time);
        } else {
            if ((int)elapsed >= delay && delay != -1 &&
                (!is_event || !is_top_event_ready(iosvc))) {
                timeout_first(iosvc, is_event, completion_time);
            }
            
            // Iterate through ready events
            dispatch_ready_events(iosvc, completion_time);
        }
        
    }

    io_errcode retval = (iosvc->status == RUNNING) ? EIO_OK : EIO_STOPPED;
    iosvc->status = DONE;

    return retval;
    /**
     *  while must-run
     *      call all posted handlers
     *
     *      poll_timeout = min(heap_delay.top(), heap_events.top())
     *
     *      poll
     *      if ready_cnt != 0
     *          foreach ready event:
     *              write EIO_OK to status
     *
     *              if timed
     *                  remove from heap, rm heap-idx from status block in node
     *                  write remaining time to time_ptr
     *              clear all fields from node
     *              call handler
     *
     *              if no more handlers for this fd
     *                  remove node
     *                  swap last of pollfds to current pos (update pollidx in
     * node of last) skip iteration index increment else (ready_cnt == 0) if
     * timeout from event-heap: write EIO_TIMEOUT to status remove from heap,
     * clear status block fields write 0 to remaining time call handler
     *
     *              if no more handlers for this fd
     *                  remove node
     *                  swap last of pollfds to current pos (update pollidx in
     * node of last) skip iteration index increment else write EIO_OK remove
     * from timed-heap call handler (or add to sync list)
     *
     *
     */
}
