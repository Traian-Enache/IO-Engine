#ifndef IOTYPES_H_
#define IOTYPES_H_ 1

/**
 * @brief Wait type to be associated with a file descriptor in an event.
 * consists of `WAIT_READ`, to signal read-related events (fd readable, EOF),
 * `WAIT_WRITE` to signal that the FD may be written to, or `WAIT_EXCEPTION`
 * for various exceptional events, such as priority reads on a socket, or
 * changes to a cgroup file
 */
typedef enum
{
    WAIT_READ,
    WAIT_WRITE,
    WAIT_EXCEPTION
} io_wait_type;

/**
 * @brief Error codes returned by various library calls under diverse
 * circumstances. Documented for each use.
 * 
 * The non-error status `EIO_OK` is the only value of this type that
 * evaluates to false in boolean-requiring contexts (such as if statement
 * condition expressions), enabling the idiom:
 * 
 * @code
 * io_errcode errc = some_io_op();
 * if (errc) {
 *     // handle error
 * }
 * @endcode
 */
typedef enum
{
    EIO_OK = 0,
    EIO_TIMEOUT,
    EIO_CANCELLED,
    EIO_STOPPED,
    EIO_INVARG,
    EIO_NOENTRY,
    EIO_NOMEM,
    EIO_INPROGRESS,
    EIO_SYSERR,
    EIO_EOF
} io_errcode;

/**
 * @brief Aggregate between a file descriptor and a `io_wait_type`. Passed to
 * `iosvc_sched()` and `iosvc_sched_timed()` to schedule a callback upon an
 * event, that will be called by the event loop (i.e. `iosvc_run()`) when
 * it becomes signalled.
 * 
 * Consists of members `fd` and `wait_type`.
 */
typedef struct {
    int fd;
    io_wait_type wait_type;
} io_event;

/**
 * @brief Aggregate of a callback (function pointer) and an associated
 * user-defined context. Called by `iosvc_run()`, according to the circumstances
 * of the function that was used to schedule this handler (i.e. calling at the
 * next iteration of an event-loop, or when an associated event is triggered).
 * 
 * Consists of `callback`, a function pointer returning `void` and taking a
 * `void*`, and `ctx`, the void pointer that will be passed as argument to the
 * callback
 */
typedef struct {
    void (*callback)(void *);
    void *ctx;
} io_handler;

/**
 * @brief Converts an error code to a human-readable C-string
 * 
 * @param errc error code
 * @return `const char*` to a null-terminated string containing the name
 * of the error. No freeing is required
 */
const char *ioec_strerr(io_errcode errc);

#endif // IOTYPES_H_
