#ifndef IO_COROUTINE_H_
#define IO_COROUTINE_H_ 1

typedef int coro_state;
#define CORO_INITIALIZER 0

inline static int coro_is_done(coro_state state) {
    return state == -1;
}

// clang-format off

/**
 * @brief Starts an asynchronous block. Specify state by value, not by pointer
 * i.e. @code
 * typedef struct {
 *     void *usr_data;
 *     coro_state state;
 * } ctx_t;
 * 
 * void my_coro(void *arg) {
 *     io_handler this_coro = {my_coro, arg};
 *     ctx_t *ctx = (ctx_t *)arg;
 * 
 *     async (ctx->state) {
 *         await iosvc_sched(svc, ..., this_coro, ...);
 * 
 *         ...
 *         await async_write_some(svc, ..., this_coro);
 *     }
 * }
 * @endcode
 * 
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

// clang-format on

#endif // IO_COROUTINE_H_
