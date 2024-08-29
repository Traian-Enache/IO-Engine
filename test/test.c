#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

// #include "src/dynarray.h"
// #include "src/heaputils.h"
#include "io_service.h"
#include "src/rbnode.h"
#include "src/iosvc_def.h"
#include "src/iosvc_dequeue.h"

#define DUMMY_FUNC ((void(*)(void*))0x1234)

int main() {
    io_errcode errc1, errc2, errc3;
    io_service *iosvc = iosvc_create();
    iosvc->status = RUNNING;
    io_errcode out;
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ}, (io_handler){DUMMY_FUNC, (void *)3}, &errc1);
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_EXCEPTION}, (io_handler){DUMMY_FUNC, (void*)6}, &errc3);
    // out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_EXCEPTION}, (io_handler){DUMMY_FUNC, (void*)6}, &errc3);
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_WRITE}, (io_handler){DUMMY_FUNC, (void*)4}, &errc2);

    // "Remove" wait_read, keeping stored write
    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ});
    // out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ});
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ}, (io_handler){DUMMY_FUNC, (void*)15}, &errc1);
    // out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ}, (io_handler){DUMMY_FUNC, (void*)15}, &errc1);

    // "Remove" both wait_read and wait_write
    // iosvc->async_handlers->main_event.handler.callback = NULL;
    // iosvc->async_handlers->aux_event->handler.callback = NULL;
    // iosvc->async_handlers->main_ev_type = EMPTY;
    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ});
    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_WRITE});
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_WRITE}, (io_handler){DUMMY_FUNC, (void*)923}, &errc3);

    // iosvc->async_handlers->ex_event->handler.callback = NULL;
    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_EXCEPTION});

    int tmout = 10000;

    out = iosvc_sched_timeout(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ}, (io_handler){DUMMY_FUNC, (void*)64}, &errc1, &tmout);
    tmout = 6000;
    out = iosvc_sched_timeout(iosvc, (io_event){.fd =3, .wait_type = WAIT_EXCEPTION}, (io_handler){DUMMY_FUNC, (void*)128}, &errc2, &tmout);

    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_READ});
    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_WRITE});
    out = iosvc_cancel(iosvc, (io_event){.fd =3, .wait_type = WAIT_EXCEPTION});

    iosvc_delete(iosvc);
    (void)out;
    return 0;
}
