#ifndef IOSVC_DEF_H_
#define IOSVC_DEF_H_ 1

#include "io_service.h"

#include "dynarray.h"
#include "cbuffer.h"
#include "rbnode.h"

struct io_service {
    cbuffer sync_handlers;
    dynarray timed_handlers_heap;

    dynarray pollfds;
    dynarray timed_events_heap;

    rb_node *async_handlers;

    enum {
        READY,
        RUNNING,
        STOPPING,
        DONE
    } status;
};

#endif // IOSVC_DEF_H_
