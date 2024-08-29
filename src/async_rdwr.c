#include "async_rdwr.h"

#include <unistd.h>
#include <stdlib.h>

typedef struct {
    io_service *iosvc;
    io_handler hnd;
    void *buf;
    size_t nbytes;
    size_t *transferred;
    io_errcode *errc;
    int fd;
    io_wait_type op_type;
} rw_ctx;

static void rw_some_impl(void *arg) {
    rw_ctx *ctx = (rw_ctx *)arg;

    if (*ctx->errc == EIO_OK) {
        ssize_t bytes_read = ctx->op_type == WAIT_READ ?
            read(ctx->fd, ctx->buf, ctx->nbytes) :
            write(ctx->fd, ctx->buf, ctx->nbytes);

        if (bytes_read < 0) {
            *ctx->errc = EIO_SYSERR;
            *ctx->transferred = 0;
        } else {
            *ctx->transferred = (size_t)bytes_read;
        }
    }

    ctx->hnd.callback(ctx->hnd.ctx);
    free(ctx);
}

static void rw_impl(void *arg) {
    rw_ctx *ctx = (rw_ctx *)arg;

    if (*ctx->errc == EIO_OK) {
        ssize_t bytes_transferred = ctx->op_type == WAIT_READ ?
            read(ctx->fd, ctx->buf, ctx->nbytes) :
            write(ctx->fd, ctx->buf, ctx->nbytes);
        
        if (bytes_transferred <= 0) {
            *ctx->errc = bytes_transferred == 0 ? EIO_EOF : EIO_SYSERR;
        } else {
            *ctx->transferred += (size_t)bytes_transferred;
            ctx->nbytes -= (size_t)bytes_transferred;
            ctx->buf = (char *)ctx->buf + bytes_transferred;

            // Check if there is more to transfer
            if (ctx->nbytes > 0) {
                io_errcode errc =
                    iosvc_sched(ctx->iosvc,
                                (io_event){ctx->fd, ctx->op_type},
                                (io_handler){rw_impl, arg},
                                ctx->errc);
            
                if (errc)
                    *ctx->errc = errc;
                else
                    return;
            }
        }
    }

    ctx->hnd.callback(ctx->hnd.ctx);
    free(ctx);
}

io_errcode async_rw_sched(io_service *iosvc, int fd, void *buf, size_t nbytes,
                          io_handler const *hnd, size_t *transferred,
                          io_errcode *errc, io_wait_type op_type,
                          void (*impl_callback)(void *)) {
    rw_ctx *ctx = (rw_ctx *)malloc(sizeof(*ctx));
    if (!ctx)
        return EIO_NOMEM;
    
    *ctx = (rw_ctx){
        .iosvc = iosvc,
        .buf = buf,
        .nbytes = nbytes,
        .hnd = *hnd,
        .errc = errc,
        .fd = fd,
        .transferred = transferred,
        .op_type = op_type
    };

    *transferred = 0;

    return iosvc_sched(iosvc, 
                       (io_event){fd, op_type},
                       (io_handler){impl_callback, ctx},
                       errc);
}

io_errcode async_read_some(io_service *iosvc, int fd, void *buf,
                           size_t nbytes, io_handler hnd,
                           size_t *transferred, io_errcode *errc) {
    return async_rw_sched(iosvc, fd, buf, nbytes, &hnd, transferred,
                          errc, WAIT_READ, rw_some_impl);
}

io_errcode async_read(io_service *iosvc, int fd, void *buf, size_t nbytes,
                      io_handler hnd, size_t *transferred, io_errcode *errc) {
    return async_rw_sched(iosvc, fd, buf, nbytes, &hnd, transferred,
                          errc, WAIT_READ, rw_impl);
}

io_errcode async_write_some(io_service *iosvc, int fd, void const *buf,
                            size_t nbytes, io_handler hnd,
                            size_t *transferred, io_errcode *errc) {
    return async_rw_sched(iosvc, fd, (void *)buf, nbytes, &hnd, transferred,
                          errc, WAIT_WRITE, rw_some_impl);
}

io_errcode async_write(io_service *iosvc, int fd, void const *buf,
                       size_t nbytes, io_handler hnd, size_t *transferred,
                       io_errcode *errc) {
    return async_rw_sched(iosvc, fd, (void *)buf, nbytes, &hnd, transferred,
                          errc, WAIT_WRITE, rw_impl);
}
