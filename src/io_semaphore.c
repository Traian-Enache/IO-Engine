#include "io_semaphore.h"

#include <stdlib.h>

typedef struct pending_handler_node {
    struct pending_handler_node *next;
    io_handler hnd;
} pending_handler_node;

struct _io_sem_t {
    io_service *iosvc;
    pending_handler_node *handler_list;
    int sem_val;
};

io_semaphore *iosem_create(int init_val, io_service *iosvc) {
    io_semaphore *iosem = (io_semaphore *)malloc(sizeof(*iosem));

    if (iosem)
        *iosem = (io_semaphore) {
            .iosvc = iosvc,
            .handler_list = NULL,
            .sem_val = init_val
        };

    return iosem;
}

void iosem_delete(io_semaphore *iosem) {
    while (iosem->handler_list) {
        pending_handler_node *curr = iosem->handler_list;
        iosem->handler_list = curr->next;

        (void)iosvc_post(iosem->iosvc, curr->hnd);
        free(curr);
    }

    free(iosem);
}

io_errcode iosem_wait(io_semaphore *iosem, io_handler hnd) {
    if (!hnd.callback)
        return EIO_INVARG;

    if (iosem->sem_val > 0) {
        --iosem->sem_val;
        return iosvc_post(iosem->iosvc, hnd);
    }

    pending_handler_node *curr_hnd =
        (pending_handler_node *)malloc(sizeof(*curr_hnd));
    
    if (!curr_hnd)
        return EIO_NOMEM;

    curr_hnd->hnd = hnd;
    curr_hnd->next = iosem->handler_list;
    iosem->handler_list = curr_hnd;

    return EIO_OK;
}

io_errcode iosem_signal(io_semaphore *iosem) {
    if (iosem->sem_val < 0 || iosem->handler_list == NULL) {
        ++iosem->sem_val;
        return EIO_OK;
    }

    pending_handler_node *to_sched = iosem->handler_list;
    iosem->handler_list = to_sched->next;

    io_errcode errc = iosvc_post(iosem->iosvc, to_sched->hnd);
    free(to_sched);

    return errc;
}
