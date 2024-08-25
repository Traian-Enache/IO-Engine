#ifndef IO_COROUTINE_H_
#define IO_COROUTINE_H_ 1

typedef int coro_state;
#define CORO_INITIALIZER 0

inline static int coro_is_done(coro_state state) {
    return state == -1;
}

// clang-format off

/**
 * @brief Starts an asynchronous block. Specify state by value, not by pointer.
 * Local variables may NOT be created in such a block; add any such variables
 * to a context object provided to the current function, as in the example:
 * @code
 * typedef struct {
 *     void *local;
 *     io_service *svc;
 *     coro_state state;
 * } ctx_t;
 * 
 * void my_coro(void *arg) {
 *     io_handler this_coro = {my_coro, arg};
 *     ctx_t *ctx = (ctx_t *)arg;
 * 
 *     async (ctx->state) {
 *         await iosvc_sched(ctx->svc, ..., this_coro, ...);
 * 
 *         ctx->local = ...;
 *         ...
 *         await async_write_some(ctx->svc, ..., this_coro);
 *     }
 * 
 *     if (coro_is_done(ctx->coro))
 *         do_cleanup();
 * }
 * @endcode
 */
#define async(state)                                \
    for (coro_state * const _co_st = &(state);      \
        !coro_is_done(*_co_st); *_co_st = -1)       \
        if (*_co_st == -2) { /* -2 is unreachable*/ \
            goto _coro_cont;                        \
    _coro_cont:                                     \
            continue;                               \
        } else if (*_co_st == -3) { /* so is -3 */  \
            goto _coro_break;                       \
    _coro_break:                                    \
            break;                                  \
        } else                                      \
            switch (*_co_st)                        \
            case 0:
            
#if defined(__COUNTER__)
#define CO_LABEL__ (__COUNTER__ + 1)
#else
#define CO_LABEL__ __LINE__
#endif

/**
 * @brief Execute the expression following the await statement,
 * save coroutine state and exit the async block. A re-entry in
 * the block will resume the coroutine from the point following
 * the await statement.
 * 
 * The argument may also be a block statement. This allows usage
 * of local variables:
 * 
 * @code
 * async (some_coro_state) {
 *     ...
 *     await {
 *         io_errcode st = iosvc_sched(..., this_coro, ...);
 *         // do something with st
 *     }
 * }
 * @endcode
 */
#define await AWAIT_IMPL(CO_LABEL__)

#define AWAIT_IMPL(label)                           \
    for (*_co_st = (label);;)                       \
        if (*_co_st == -4) {                        \
            case (label):                           \
                break;                              \
        } else                                      \
            switch (0)                              \
                for (;;)                            \
                    if (*_co_st >= -1)  /* true */  \
                        goto _coro_cont;            \
                    else                            \
                        for (;;)                    \
                            if (*_co_st >= -2)      \
                                goto _coro_break;   \
                            else /* fallthrough */  \
                            case 0:

/**
 * @brief marks the current coroutine as finished, such that an
 * async block may not execute when entered, and coro_is_done()
 * returns a nonzero value.
 * Usage: @code
 * async (some_coro_state) {
 *     ...
 *     if (some_condition)
 *         co_return;
 * }
 * @endcode
 */
#define co_return await break

// clang-format on

#endif // IO_COROUTINE_H_
