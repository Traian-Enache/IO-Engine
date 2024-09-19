# POSIX Asynchronous IO Engine

## Single-threaded I/O asynchronous reactor for POSIX compliant platforms

This library aims to provide a lightweight abstraction on top of the low-level asynchronous
IO APIs provided by the POSIX standard, without creating *any* background threads.

This library's interface is partially inspired by Boost.ASIO: It provides an execution context (`io_service`) where the user schedules asynchronous jobs, and calls `iosvc_run()` to await completion of all tasks.

## Using and building this library

To use this library, add the `include` folder to both your IDE's and your compiler of choice's include path.

To build this library, run `make` in the root directory. A statically linked library will be generated, under the name `libioeng.a`.

## Header Synopsis and Documentation

### `iotypes.h`

This header contains the following two structures: `io_event` and `io_handler`. They are the low-level building blocks of more abstract asynchronous IO operations, and are widely used throughout the library.
* `io_event` is essentially a pair of a file descriptor and a library defined `io_wait_type`, an enumerator containing three values, according to what kind of event should be awaited for the file descriptor
* `io_handler`, a pair of a pointer to a function returning `void` and taking a `void*` parameter (i.e. `void(*)(void *)`) and another void pointer. Oftentimes paired with an `io_event`, this represents a completion handler that should be called when the event is triggered (passing the contained void pointer to the contained function, to provide context).

To indicate the success/error status of the various library functions, most return an `io_errcode`, specifying what kind of error occurred. Refer to the documentation of each function to see possible values.

### `io_service.h`

This represents the workhorse of the library, containing the aforementioned opaque `io_service` type, as well as the functions that operate on it (i.e. `iosvc_*()`).

It contains the following functions:
* `iosvc_create()` and `iosvc_destroy()`, which serve as constructor and destructor for the `io_service` type, respectively
* `iosvc_post()` is used to schedule a completion handler (i.e. `io_handler`) to run as soon as possible in the event loop
* `iosvc_sched()` schedules a completion handler to be run when an asynchronous event occurs
* `iosvc_post_delay()` and `iosvc_sched_timeout()` are analogous to their non-timed counterparts: the former runs the handler after a specified delay from the moment of the function call, and the latter calls the handler if the event is not signalled within the specified duration.

Note: all of the handler-scheduling functions presented, except for `iosvc_post()` use an out parameter error code that will be asynchronously set, just before the call to the handler. This provides context as to why the handler was called (e.g. called normally, timeout, via explicit cancellation, etc.)

* `iosvc_cancel()` cancels a scheduled `io_event`, calling its associated callback, and mentioning that the callback was called as a result of a cancellation via the error code out-parameter provided when the event was scheduled
* `iosvc_run()`, which runs the event loop, calling all `post`ed handlers and awaiting all the events to occur. Completion handlers called in the event loop may schedule waits for other events, to chain asynchronous operations. This function returns when all pending handlers have been called.
* `iosvc_stop()` requests the service to stop, calling all scheduled callbacks and providing context (via error code) that the service has stopped.
* `iosvc_reset()` prepares a stopped service to be reused.

Refer to the various tests under the `test` directory for usage examples.

### `coroutine.h`

Provides implementation for stackless coroutines (a.k.a async/await functions), via the following constructs:

* An `async` keyword, used to start an *async block*
* A `coro_state` type, used as an argument to the async block, which holds the suspension/resumption point of the coroutine
* An `await` statement, used to execute a statement and suspend the current coroutine (i.e. create a suspension point right after it, saves the coroutine state so that a re-entry in the async block resumes execution at the associated suspension point, and jumps out of the async block).

Refer to the Doxygen comments in the code for further details, and to "`test/testsem.c`" for an example of usage.

### `io_semaphore.h`

Provides a semaphore to be used within completion handlers. This is especially useful in conjunction with coroutines.

Exposes the type `io_semaphore`, as well as the functions that operate on it (`iosem_create()`, `iosem_delete()`, `iosem_wait()` and `iosem_signal()`). Refer to the Doxygen comments in the code for further details.

### `async_rdwr.h`

Provides asynchronous read/write primitives: `async_read()`, `async_read_some()`, `async_write()` and `async_write_some()`. These are meant to be composed in order to create higher-level functions.

### `async_net.h`

Exposes asynchronous connect and accept functions (i.e. `async_connect()` and `async_accept()`).

### `task_group.h`

Implements task groups, used to schedule a completion handler after multiple asynchronous tasks have completed.

## Extending the functionality of this library

To build higher-level asynchronous functions, there are two approaches:

### Callback chaining

In this method, a completion handler schedules another one on the event loop. Refer to "`src/async_rdwr.c`" for examples on how to do this, namely the `async_read()` and `async_write()` functions. These could have been implemented in terms of their `some` counterparts, but the current implementation saves on allocations.

### Coroutines

The preferred method for asynchronous function composition. Refer to `test/testsem.c` for an example of coroutine creation (as well as semaphore usage).
