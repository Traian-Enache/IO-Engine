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


void cb(void*) {}

int main() {
    io_service *iosvc = iosvc_create();
    io_errcode errc1, errc2, errc3, out;
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_EXCEPTION}, (io_handler){cb, (void*)6}, &errc3);
    out = iosvc_sched(iosvc, (io_event){.fd =9, .wait_type = WAIT_READ}, (io_handler){cb, (void*)55}, &errc2);
    out = iosvc_sched(iosvc, (io_event){.fd =3, .wait_type = WAIT_WRITE}, (io_handler){cb, (void*)15}, &errc1);

    (void)out;
    return 0;
}