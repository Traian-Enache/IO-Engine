#include "io_service.h"
#include "io_semaphore.h"
#include "coroutine.h"
#include "async_rdwr.h"

#include <stdio.h>
#include <unistd.h>

io_service *iosvc;

typedef struct {
    coro_state state;
    io_semaphore *sem;
    size_t xfer;
    io_errcode erc;
} ctx1;

char bf[128];

void coro1(void *arg) {
    ctx1 *ctx = arg;
    io_handler this_coro = {coro1, arg};

    async (ctx->state) {
        printf("awaiting signal from coro\n");

        await iosem_wait(ctx->sem, this_coro);
        printf("hello from coro1, past first await\n");

        await iosem_wait(ctx->sem, this_coro);
        printf("hello from coro1, past last await\n");

        await async_read(iosvc, STDIN_FILENO, &bf[0], 48,
                         this_coro, &ctx->xfer, &ctx->erc);
        
        printf("Read status: %s\n", ioec_strerr(ctx->erc));
        printf("nbytes: %zu, content: %*s\n", ctx->xfer, (int)ctx->xfer, bf);
    }

    if (coro_is_done(ctx->state)){
        printf("coro is done\n");
        iosvc_stop(iosvc);
    }
}

void coro2(void *arg) {
    io_semaphore *sem = arg;

    iosem_signal(sem);
    printf("signalled coro1 to run\n");
}

void stopper(void *arg) {
    (void)arg;
    // printf("canceller: %s\n", ioec_strerr(iosvc_cancel(iosvc, (io_event){0, WAIT_READ})));
    printf("canceller: %s\n", ioec_strerr(iosvc_stop(iosvc)));
}


int main() {
    iosvc = iosvc_create();
    io_semaphore *iosem = iosem_create(0, iosvc);
    ctx1 ctx = {CORO_INITIALIZER, iosem, 0, EIO_OK};

    iosvc_post_delay(iosvc, (io_handler){coro1, &ctx}, NULL, 600);
    iosvc_post_delay(iosvc, (io_handler){coro2, iosem}, NULL, 4500);
    iosvc_post_delay(iosvc, (io_handler){coro2, iosem}, NULL, 2700);
    iosvc_post_delay(iosvc, (io_handler){stopper, NULL}, NULL, 10000);

    iosvc_run(iosvc);
    iosem_delete(iosem);
    iosvc_delete(iosvc);

    return 0;
}
