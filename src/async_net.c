#include "async_net.h"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

typedef struct {
    int listen_sock;
    struct sockaddr *addr;
    socklen_t *addrlen;
    io_handler hnd;
    int *new_sock;
    io_errcode *errc;
} accept_ctx;

typedef struct {
    int sockfd;
    io_handler hnd;
    io_errcode *errc;
} conn_ctx;

static void connect_impl(void *arg) {
    conn_ctx *ctx = (conn_ctx *)arg;

    if (*ctx->errc == EIO_OK) {
        int conn_rc;
        socklen_t optlen = sizeof(conn_rc);

        if (getsockopt(ctx->sockfd, SOL_SOCKET, SO_ERROR,
                       &conn_rc, &optlen) || conn_rc)
            *ctx->errc = EIO_SYSERR;
    }
    
    ctx->hnd.callback(ctx->hnd.ctx);
    free(ctx);
}

static void accept_impl(void *arg) {
    accept_ctx *ctx = (accept_ctx *)arg;

    if (*ctx->errc == EIO_OK) {
        *ctx->new_sock = accept(ctx->listen_sock, ctx->addr, ctx->addrlen);

        if (*ctx->new_sock < 0)
            *ctx->errc = EIO_SYSERR;
    }
    
    ctx->hnd.callback(ctx->hnd.ctx);
    free(ctx);
}

io_errcode async_accept(io_service *iosvc, int listen_sock,
                        struct sockaddr *addr, socklen_t *addrlen,
                        io_handler hnd, int *new_sock, io_errcode *errc) {
    accept_ctx *ctx = (accept_ctx *)malloc(sizeof(*ctx));
    if (!ctx)
        return EIO_NOMEM;

    *ctx = (accept_ctx){
        .addr = addr,
        .addrlen = addrlen,
        .errc = errc,
        .hnd = hnd,
        .listen_sock = listen_sock,
        .new_sock = new_sock
    };

    return iosvc_sched(iosvc, 
                       (io_event){.fd = listen_sock, .wait_type = WAIT_READ},
                       (io_handler){accept_impl, ctx},
                       errc);
}

io_errcode async_connect(io_service *iosvc, int sockfd,
                         struct sockaddr const *addr, socklen_t addrlen,
                         io_handler hnd, io_errcode *errc) {
    // Set as nonblocking
    int rc = fcntl(sockfd, F_GETFL, 0);
    if (rc == -1)
        return EIO_SYSERR;
    rc = fcntl(sockfd, F_SETFL, rc | O_NONBLOCK);
    if (rc == -1)
        return EIO_SYSERR;
    
    conn_ctx *ctx = (conn_ctx *)malloc(sizeof(*ctx));
    if (!ctx)
        return EIO_NOMEM;
    
    *ctx = (conn_ctx){
        .errc = errc,
        .hnd = hnd,
        .sockfd = sockfd
    };

    rc = connect(sockfd, addr, addrlen);
    if (rc == -1 && errno != EINPROGRESS) {
        free(ctx);
        return EIO_SYSERR;
    }

    return iosvc_sched(iosvc, 
                       (io_event){.fd = sockfd, .wait_type = WAIT_WRITE},
                       (io_handler){connect_impl, ctx},
                       errc);
}
