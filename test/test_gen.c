#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>

#include "io_service.h"
#include <unistd.h>

io_service *iosvc;
int prm1 = 9;
int prm2 = 22;

int tfd;

void cbend(void *ptr) {
    printf("endcb with param = %d\n", *(int *)ptr);
}

void cb(void *ptr) {
    printf("called cb with param = %d\n", *(int *)ptr);

    io_errcode errc = iosvc_post_delay(iosvc, (io_handler){cbend, &prm1}, NULL, 999);
    printf("calling cbend with param = %d, errc = %d\n", prm1, errc);
}

int rpram = 3;
io_errcode erc;
void rcb(void *ctx) {
    printf("Available data to read (st=%d)! ", erc);
    char buf[128];
    int rtv = read(STDIN_FILENO, buf, 128);
    buf[rtv] = '\0';

    int *rds = (int *)ctx;
    printf("%s\nReads remaining: %d\n", buf, --(*rds));

    if (*rds) {
        iosvc_sched(iosvc,
            (io_event){STDIN_FILENO, WAIT_READ},
            (io_handler){rcb, ctx},
            &erc);
    }
}

int texp = 4;
void tcb(void *ctx) {
    int *ictx = (int *)ctx;
    printf("Timer expired\nRemaining expirations: %d\n", --(*ictx));
    uint64_t tbuf;
    (void)read(tfd, &tbuf, 8);
    if (*ictx) {
        iosvc_sched(iosvc,
            (io_event){tfd, WAIT_READ},
            (io_handler){tcb, &texp},
            NULL);
    }
}

int main() {
    int param = 8;
    tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    iosvc = iosvc_create();
    struct itimerspec itm = {{2,0},  {1,500000000}};
    timerfd_settime(tfd, 0, &itm, NULL);

    iosvc_sched(iosvc,
            (io_event){tfd, WAIT_READ},
            (io_handler){tcb, &texp},
            NULL);
    iosvc_sched(iosvc,
            (io_event){STDIN_FILENO, WAIT_READ},
            (io_handler){rcb, &rpram},
            &erc);

    iosvc_run(iosvc);
    iosvc_delete(iosvc);
    close(tfd);
    return 0;
}
