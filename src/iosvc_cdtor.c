#include "io_service.h"
#include "iosvc_def.h"

#include <poll.h>

#include "async_heap_entry.h"
#include "delay_heap_entry.h"

io_service *iosvc_create() {
    io_service *iosvc = (io_service *)malloc(sizeof(*iosvc));
    if (!iosvc)
        return NULL;

    cbuf_init(&iosvc->sync_handlers, sizeof(io_handler));
    dynarr_init(&iosvc->pollfds, sizeof(struct pollfd));

    iosvc->async_handlers = NULL;
    iosvc->status = READY;

    dynarr_init(&iosvc->timed_events_heap, sizeof(async_heap_entry));
    dynarr_init(&iosvc->timed_handlers_heap, sizeof(delay_heap_entry));

    return iosvc;
}

static void del_rb_node(rb_node *node) {
    if (!node)
        return;

    free(node->aux_event);
    free(node->ex_event);

    del_rb_node(node->left);
    del_rb_node(node->right);

    free(node);
}

void iosvc_delete(io_service *iosvc) {
    dynarr_delete(&iosvc->timed_events_heap);
    dynarr_delete(&iosvc->timed_handlers_heap);

    dynarr_delete(&iosvc->pollfds);
    cbuf_delete(&iosvc->sync_handlers);

    del_rb_node(iosvc->async_handlers);
    free(iosvc);
}
