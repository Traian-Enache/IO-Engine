#include "iosvc_def.h"

#include <time.h>
#include <poll.h>

#include "heaputils.h"
#include "async_heap_entry.h"

#include "rbtree.h"

#define READ_MASK   (POLLRDNORM | POLLRDBAND | POLLERR | POLLHUP | POLLIN)
#define WRITE_MASK  (POLLWRNORM | POLLWRBAND | POLLERR | POLLOUT)
#define EXCEPT_MASK (POLLPRI)

/**
 * @brief Check if an event is scheduled in the given node
 * 
 * @param node node to query for event presence
 * @param event event to check
 * @return 0 if not found, nonzero if present
 */
inline static int is_scheduled(rb_node *node, io_event event) {
    if (event.wait_type == WAIT_EXCEPTION)
        return node->ex_event && node->ex_event->handler.callback;

    if (node->main_ev_type == EMPTY)
        return 0;

    // Check if requested event is scheduled as main event
    if ((event.wait_type == WAIT_READ && node->main_ev_type == READ) ||
        (event.wait_type == WAIT_WRITE && node->main_ev_type == WRITE)) {
        return node->main_event.handler.callback != NULL;
    } else {
        return node->aux_event && node->aux_event->handler.callback;
    }
}

/**
 * @brief Reserve memory for an event in the current node
 * 
 * @param node node to reserve memory in
 * @param requested `WAIT_READ` or `WAIT_WRITE`
 * @return `NULL` if no memory could be allocated, non-null pointer
 * to reserved memory if successful
 */
inline static event_data *reserve_rd_wr(rb_node *node, io_wait_type requested) {
    // Check if no R/W event is stored, if so, use inline storage
    if (node->main_ev_type == EMPTY) {
        node->main_ev_type = requested == WAIT_READ ? READ : WRITE;

        return &node->main_event;
    }

    io_wait_type stored = node->main_ev_type == READ ? WAIT_READ : WAIT_WRITE;

    if (stored == requested)
        return &node->main_event;

    // stored != requested, first check if there is allocated space that
    // can be reused, else check if exception is not in use and can be stolen
    if (!node->aux_event) {
        if (node->ex_event && !node->ex_event->handler.callback) {
            node->aux_event = node->ex_event;
            node->ex_event = NULL;
        } else {
            node->aux_event = (event_data *)malloc(sizeof(*node->aux_event));
        }
    }

    return node->aux_event;
}
/**
 * @brief Reserve and allocate memory for event data (optionally a node too if
 * provided event's FD has no node)
 * 
 * @param place place of node in set
 * @param parent parent of node in set
 * @param root_ref reference to root of set
 * @param event requested event
 * @return A valid, in place pointer for success, or NULL if out of mem 
 */
inline static event_data *reserve_evt_mem(rb_place place, rb_node *parent,
                                          rb_node **root_ref, io_event event) {
    int is_new_node = (*place == NULL);
    rb_node *new_node;

    // Allocate node if needed
    if (is_new_node) {
        if ((new_node = make_rbnode(event.fd)) == NULL)
            return NULL;
    } else {
        new_node = *place;
    }

    event_data *retval;
    if (event.wait_type == WAIT_EXCEPTION) {
        if (!new_node->ex_event) {
            // No storage for exception, check if aux is allocated and not in
            // use and steal its memory
            if (new_node->aux_event && !new_node->aux_event->handler.callback) {
                new_node->ex_event = new_node->aux_event;
                new_node->aux_event = NULL;
            } else {
                new_node->ex_event = (event_data *)malloc(sizeof(*retval));
            }
        }

        retval = new_node->ex_event;
    } else {
        retval = reserve_rd_wr(new_node, event.wait_type);
    }

    if (is_new_node) {
        if (retval) {
            rb_insert(new_node, place, parent, root_ref);
            *place = new_node;
        } else {
            free(new_node);
        }
    }

    *place = new_node;
    return retval;
}

inline static io_errcode iosvc_enqueue(io_service *iosvc, io_event event,
                                       io_handler *phnd, io_errcode *status,
                                       rb_node **out_node) {
    if (iosvc->status != READY && iosvc->status != RUNNING)
        return EIO_STOPPED;

    if (!phnd->callback || event.wait_type == 3u)
        return EIO_INVARG;

    rb_node *parent = NULL;
    rb_place place = rb_probe(&iosvc->async_handlers, &parent, event.fd);
    int is_new_node = (*place == NULL);

    if (!is_new_node) {
        if (is_scheduled(*place, event))
            return EIO_INPROGRESS;
    } else {
        if (dynarr_emplace_back(&iosvc->pollfds) == NULL)
            return EIO_NOMEM;
    }

    // Check for vacated storage (by previous dequeue if any), or allocate
    // memory for requested event if no vacant storage is found
    // (optionally allocate node too if required)
    event_data *data =
        reserve_evt_mem(place, parent, &iosvc->async_handlers, event);

    if (!data) {
        // Failed to allocate data for event
        if (is_new_node)
            dynarr_pop_back(&iosvc->pollfds);

        return EIO_NOMEM;
    }

    *data = (event_data){
        .ddl_heap_idx = -1,
        .handler = *phnd,
        .status = status
    };

    *out_node = *place;
    struct pollfd *pollent_ptr = NULL;

    // Find pollfd entry in pollfds array, or create one if needed
    if (is_new_node) {
        (*out_node)->pollfd_idx = dynarr_size(&iosvc->pollfds) - 1;

        pollent_ptr = (struct pollfd *)dynarr_back(&iosvc->pollfds);
        *pollent_ptr = (struct pollfd){
            .fd = event.fd,
            .events = 0,
            .revents = 0,
        };
    } else {
        pollent_ptr = (struct pollfd *)
                        dynarr_at(&iosvc->pollfds, (*out_node)->pollfd_idx);
    }

    static short const evt_masks[] = {
        [WAIT_READ]      = READ_MASK,
        [WAIT_WRITE]     = WRITE_MASK,
        [WAIT_EXCEPTION] = EXCEPT_MASK
    };

    pollent_ptr->events |= evt_masks[event.wait_type];

    return EIO_OK;
}

io_errcode iosvc_sched(io_service *iosvc, io_event event, io_handler hnd,
                       io_errcode *status) {
    rb_node *res_node;
    io_errcode ioerr = iosvc_enqueue(iosvc, event, &hnd, status, &res_node);
    (void)res_node;

    return ioerr;
}


io_errcode iosvc_sched_timeout(io_service *iosvc, io_event event,
                               io_handler hnd, io_errcode *status,
                               int *milliseconds)
{
    int64_t deadline = current_time() + *milliseconds;

    async_heap_entry *heap_ent =
        (async_heap_entry *)dynarr_emplace_back(&iosvc->timed_events_heap);

    if (!heap_ent)
        return EIO_NOMEM;

    rb_node *res_node;
    io_errcode ioerr = iosvc_enqueue(iosvc, event, &hnd, status, &res_node);
    if (ioerr) {
        dynarr_pop_back(&iosvc->timed_events_heap);
        return ioerr;
    }

    *heap_ent = (async_heap_entry){
        .deadline = deadline,
        .event_type = event.wait_type,
        .io_op_data = res_node,
        .remaining = milliseconds
    };

    size_t heap_idx = dynarr_size(&iosvc->timed_events_heap) - 1;
    get_evt_data(res_node, event.wait_type)->ddl_heap_idx = (ptrdiff_t)heap_idx;

    heap_sift_up_idx(&iosvc->timed_events_heap, heap_idx,
                     async_hp_ent_lt, async_hp_ent_swp);

    return ioerr;
}
