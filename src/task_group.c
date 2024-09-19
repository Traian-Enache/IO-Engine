#include "task_group.h"

#include <stdlib.h>

typedef struct {
    size_t remaining_tasks;
    io_handler hnd;
} task_group_ctx;

static void task_group_callback(void *arg) {
    task_group_ctx *ctx = (task_group_ctx *)arg;

    if (--ctx->remaining_tasks == 0) {
        ctx->hnd.callback(ctx->hnd.ctx);
        free(arg);
    }
}

io_handler make_task_group(size_t num_tasks, io_handler hnd) {
    task_group_ctx *ctx = (task_group_ctx *)malloc(sizeof(*ctx));

    if (!ctx)
        return (io_handler){NULL, NULL};

    ctx->hnd = hnd;
    ctx->remaining_tasks = num_tasks;

    return (io_handler){task_group_callback, ctx};
}
