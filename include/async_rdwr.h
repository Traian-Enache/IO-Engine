#ifndef ASYNC_RDWR_H_
#define ASYNC_RDWR_H_ 1

#include "io_service.h"
#include "stddef.h"

/**
 * @brief Schedule an asynchronous read of at most `nbytes` from `fd` into `buf`
 * on the service `iosvc`, and call the provided handler when done.
 * 
 * @param iosvc service to schedule read on
 * @param fd file descriptor to read from
 * @param buf buffer to read into
 * @param nbytes maximum number of bytes to transfer
 * @param hnd completion handler
 * @param transferred out parameter; number of bytes actually read is stored in
 * the location pointed by this parameter
 * @param errc out parameter; stores the operation's completion status
 *
 * @return `EIO_OK` The operation has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A read has already been issued for this `fd`
 * 
 * Reported status through `errc` may be:
 * 
 * `EIO_OK` The read has completed successfully
 * 
 * `EIO_INVARG` The file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 * 
 * `EIO_SYSERR` The read failed. Cause is found via inspecting `errno`
 */
io_errcode async_read_some(io_service *iosvc, int fd, void *buf,
                           size_t nbytes, io_handler hnd,
                           size_t *transferred, io_errcode *errc);

/**
 * @brief Schedule an asynchronous write of at most `nbytes` into `fd` from `buf`
 * on the service `iosvc`, and call the provided handler when done.
 * 
 * @param iosvc service to schedule write on
 * @param fd file descriptor to write to
 * @param buf buffer to write from
 * @param nbytes maximum number of bytes to transfer
 * @param hnd completion handler
 * @param transferred out parameter; number of bytes actually written is stored
 * in the location pointed by this parameter
 * @param errc out parameter; stores the operation's completion status
 * 
 * @return `EIO_OK` The operation has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A write has already been issued for this `fd`
 * 
 * Reported status through `errc` may be:
 * 
 * `EIO_OK` The read has completed successfully
 * 
 * `EIO_INVARG` The file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 * 
 * `EIO_SYSERR` The write failed. Cause is found via inspecting `errno`
 */
io_errcode async_write_some(io_service *iosvc, int fd, void const *buf,
                            size_t nbytes, io_handler hnd,
                            size_t *transferred, io_errcode *errc);

/**
 * @brief Schedule an asynchronous read of exactly `nbytes` from `fd` into `buf`
 * on the service `iosvc`, and call the provided handler when done. If `errc` is
 * set to a value other than `EIO_OK`, the read might have provided less bytes
 * than requested.
 * 
 * @param iosvc service to schedule read on
 * @param fd file descriptor to read from
 * @param buf buffer to read into
 * @param nbytes maximum number of bytes to transfer
 * @param hnd completion handler
 * @param transferred out parameter; number of bytes actually read is stored in
 * the location pointed by this parameter
 * @param errc out parameter; stores the operation's completion status
 *
 * @return `EIO_OK` The operation has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A read has already been issued for this `fd`
 * 
 * Reported status through `errc` may be:
 * 
 * `EIO_OK` The read has completed successfully
 * 
 * `EIO_INVARG` The file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 * 
 * `EIO_SYSERR` The read failed. Cause is found via inspecting `errno`
 * 
 * `EIO_EOF` End of file/transmission has been reached
 */
io_errcode async_read(io_service *iosvc, int fd, void *buf, size_t nbytes,
                      io_handler hnd, size_t *transferred, io_errcode *errc);

/**
 * @brief Schedule an asynchronous write of exactly `nbytes` from `fd` into
 * `buf` on the service `iosvc`, and call the provided handler when done. If
 * `errc` is set to a value other than `EIO_OK`, the read might have provided
 * less bytes than requested.
 * 
 * @param iosvc service to schedule read on
 * @param fd file descriptor to read from
 * @param buf buffer to read into
 * @param nbytes maximum number of bytes to transfer
 * @param hnd completion handler
 * @param transferred out parameter; number of bytes actually read is stored in
 * the location pointed by this parameter
 * @param errc out parameter; stores the operation's completion status
 *
 * @return `EIO_OK` The operation has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A read has already been issued for this `fd`
 * 
 * Reported status through `errc` may be:
 * 
 * `EIO_OK` The read has completed successfully
 * 
 * `EIO_INVARG` The file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * event
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 * 
 * `EIO_SYSERR` The read failed. Cause is found via inspecting `errno`
 * 
 * `EIO_EOF` A write of zero bytes was attempted
 */
io_errcode async_write(io_service *iosvc, int fd, void const *buf,
                       size_t nbytes, io_handler hnd, size_t *transferred,
                       io_errcode *errc);

#endif // ASYNC_RDWR_H_
