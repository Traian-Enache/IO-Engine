#ifndef IO_SERVICE_H_
#define IO_SERVICE_H_ 1

#include "iotypes.h"

/**
 * @brief The structure responsible for multiplexing, scheduling
 * and calling of completion handlers associated with provided
 * events. For use with `iosvc_XXX()` functions;
 */
typedef struct io_service io_service;

/**
 * @brief Create a new `io_service` instance. Most programs need only a single
 * one
 * 
 * @return created instance.
 */
io_service *iosvc_create();

/**
 * @brief Releases resources held by the provided `io_service` instance.
 * 
 * @param iosvc service to be freed
 * @par Returns
 *      Nothing.
 */
void iosvc_delete(io_service *iosvc);

/**
 * @brief Runs the event loop, waiting for events and calling associated
 * completion handlers once triggered.
 * 
 * Said completion handlers may queue other completion handlers to be run
 * in the same event loop.
 * 
 * @param iosvc `io_service` whose event loop to run.
 * @return `EIO_OK` The event loop finished all scheduled tasks
 * 
 * @return `EIO_STOPPED` A call to `iosvc_stop()` was issued while the event loop
 * was running (by a completion handler)
 * 
 * @return `EIO_INPROGRESS` The event loop is already running
 * 
 * @return `EIO_INVARG` The service has finished execution and was not reset
 */
io_errcode iosvc_run(io_service *iosvc);

/**
 * @brief Stops an `io_service`'s event loop.
 *
 * The effects of this function are as follows:
 * 
 * * Reject any further completion handler scheduling requests
 * 
 * * Call any synchronous handlers (scheduled via `iosvc_post()`) that are
 * pending
 * 
 * * Call timeout handlers upon the next iteration of the event loop
 * (specifying `EIO_STOPPED` in the status argument if provided)
 * 
 * * Call async (event-based) handlers, regardless if associated event
 * had occurred, specifying `EIO_STOPPED` in the status argument if provided.
 * In the case of timeout event callbacks, remaining time is specified via
 * the in-out `remaining` parameter.
 *
 * * Cause the call to `iosvc_run()` to return, with the code `EIO_STOPPED`
 * 
 * @param iosvc service whose event loop to stop
 * @return `EIO_OK` The stop request was received successfully
 * 
 * @return `EIO_INPROGRESS` A call to stop was already made, and the service is
 * currently stopping
 * 
 * @return `EIO_INVARG` The service's event loop is not running
 */
io_errcode iosvc_stop(io_service *iosvc);

/**
 * @brief Reset a stopped or finished `io_service`, such that it can
 * be reused
 * 
 * @param iosvc service to be reset
 * @return `EIO_OK` The reset was successful, and new tasks can be scheduled
 * to be run by a subsequent `iosvc_run()` call
 * 
 * @return `EIO_INPROGRESS` The event loop is currently running
 */
io_errcode iosvc_reset(io_service *iosvc);

/**
 * @brief Schedules a handler to be synchronously executed by a service
 * as soon as possible
 * 
 * @param iosvc service to execute the handler on
 * @param hnd handler to run
 * @return `EIO_OK` The handler has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service's event loop is stopping or has stopped
 * and has not been reset
 */
io_errcode iosvc_post(io_service *iosvc, io_handler hnd);

/**
 * @brief Schedules a handler to be asynchronously executed by a service
 * when the supplied event is triggered
 * 
 * @param iosvc service to execute the handler on
 * @param event event to await for triggering
 * @param hnd handler to run
 * @param status completion status signalled by the service
 *
 * @return `EIO_OK` The handler has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A handler is already scheduled for the specified
 * event
 * 
 * If the `status` parameter is not `NULL`, the service will provide the
 * completion status for the supplied handler before calling it. This means
 * that the pointed-to variable should live at least until the handler is
 * called. Reported status might be:
 * 
 * `EIO_OK` The handler is successfully called upon triggering of the event
 * 
 * `EIO_INVARG` The event's file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 */
io_errcode iosvc_sched(io_service *iosvc, io_event event, io_handler hnd,
                       io_errcode *status);

/**
 * @brief Cancel a pending event's handler. This results in immediately 
 * calling the handler, and setting its associated `io_errcode` status to
 * `EIO_CANCELLED` (if provided).
 * 
 * @param iosvc service where the event is scheduled
 * @param event event to cancel
 * @return `EIO_OK` The event was cancelled
 * 
 * @return `EIO_NOENTRY` The specified event is not scheduled to this service
 * 
 * @return `EIO_INVARG` The service is not running
 */
io_errcode iosvc_cancel(io_service *iosvc, io_event event);

/**
 * @brief Schedules a handler to be executed after a delay.
 * 
 * @param iosvc service to schedule the handler on
 * @param hnd handler to schedule
 * @param status completion status signalled by the service
 * @param milliseconds delay to wait from the moment this function is called
 * (in milliseconds)
 * @return `EIO_OK` The handler has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * If the `status` parameter is not `NULL`, the service will provide the
 * completion status for the supplied handler before calling it. This means
 * that the pointed-to variable should live at least until the handler is
 * called. Reported status might be:
 * 
 * `EIO_OK` The handler is successfully called upon triggering of the event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 */
io_errcode iosvc_post_delay(io_service *iosvc, io_handler hnd,
                            io_errcode *status, int milliseconds);

/**
 * @brief Schedules a handler to be asynchronously executed by a service
 * when the supplied event is triggered, or when a timeout expires
 * 
 * @param iosvc service to execute the handler on
 * @param event event to await for triggering
 * @param hnd handler to run
 * @param status completion status signalled by the service
 * @param milliseconds pointer to in-out variable containing maximum timeout to
 * wait (in milliseconds). The variable should live at least until the handler
 * is called
 * 
 * Upon calling the completion handler, the remaining time until timeout
 * will be provided in the variable pointed to by the `milliseconds` parameter
 *
 * @return `EIO_OK` The handler has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A handler is already scheduled for the specified
 * event
 * 
 * If the `status` parameter is not `NULL`, the service will provide the
 * completion status for the supplied handler before calling it. This means
 * that the pointed-to variable should live at least until the handler is
 * called. Reported status might be:
 * 
 * `EIO_OK` The handler is successfully called upon triggering of the event
 * 
 * `EIO_TIMEOUT` The specified time period has expired
 * 
 * `EIO_INVARG` The event's file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 */
io_errcode iosvc_sched_timeout(io_service *iosvc, io_event event,
                               io_handler hnd, io_errcode *status,
                               int *milliseconds);

#endif // IO_SERVICE_H_
