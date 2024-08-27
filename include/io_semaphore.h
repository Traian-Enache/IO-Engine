#ifndef IO_SEMAPHORE_H_
#define IO_SEMAPHORE_H_ 1

#include "io_service.h"

typedef struct _io_sem_t io_semaphore;

/**
 * @brief Creates a semaphore to be used in a `io_service` handler context
 * 
 * @param init_val initial value of semaphore
 * @param iosvc service to associate semaphore with
 * @return A valid pointer to a semaphore, or `NULL` if failed
 */
io_semaphore *iosem_create(int init_val, io_service *iosvc);

/**
 * @brief Destroys a semaphore and schedules all handles waiting on it
 * 
 * @param iosem semaphore to destroy
 */
void iosem_delete(io_semaphore *iosem);

/**
 * @brief Schedule a handler to run when the semaphore's signal count
 * is positive
 * 
 * @param iosem semaphore
 * @param hnd handler to execute
 * 
 * @return `EIO_OK` The handler has been successfully queued
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service's event loop is stopping or has stopped
 * and has not been reset
 */
io_errcode iosem_wait(io_semaphore *iosem, io_handler hnd);

/**
 * @brief Signal the semaphore to schedule a waiting handler (or to increment 
 * signal count)
 * 
 * @param iosem semaphore to signal
 *
 * @return `EIO_OK` The semaphore has been successfully signalled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory to schedule a pending
 * handler
 * 
 * @return `EIO_STOPPED` The service's event loop is stopping or has stopped
 * and has not been reset
 */
io_errcode iosem_signal(io_semaphore *iosem);

#endif // IO_SEMAPHORE_H_
