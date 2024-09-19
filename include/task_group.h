#ifndef TASK_GROUP_H_
#define TASK_GROUP_H_ 1

#include <stddef.h>
#include "io_service.h"

/**
 * @brief 
 * Creates a completion handler that calls the passed callback `hnd`
 * after it has been invoked `num_tasks` times. To be used when an
 * operation must be executed after a group of asynchronous operations
 * are completed
 * 
 * Example usage:
 * @code
 * io_handler this_coro = {...};
 * // ...
 * 
 * await {
 *   io_handler task_group = make_task_group(4, this_coro);
 * 
 *   // Total of 4 tasks, current coroutine will be rescheduled
 *   // when all of them complete.
 * 
 *   async_read_some(..., task_group, ...);
 *   async_write(..., task_group, ...);
 *   iosvc_post_delay(ctx->svc, task_group, ...);
 *   iosem_wait(ctx->sem, task_group);
 * }
 * @endcode
 * @param num_tasks Total number of operations to await
 * @param hnd Completion handler to invoke after all tasks have finished
 * @return A valid completion handler to supply to the asynchronous operations
 * to wait for, or a handler with its `callback` set to `NULL` if no memory
 * could be allocated
 */
io_handler make_task_group(size_t num_tasks, io_handler hnd);

#endif // TASK_GROUP_H_
