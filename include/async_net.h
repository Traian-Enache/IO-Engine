#ifndef ASYNC_NET_H_
#define ASYNC_NET_H_ 1

#include <sys/socket.h>
#include "io_service.h"

/**
 * @brief Asynchronously accepts a new connection on the listening socket
 * `listen_sock`
 * 
 * @param iosvc service to schedule accept on
 * @param listen_sock listening socket
 * @param addr out parameter; the address of the connecting peer
 * @param addrlen in-out parameter; must contain the size of the memory block
 * pointed to by `addr`, and gets filled with the actual size of the address
 * @param hnd completion handler
 * @param new_sock out parameter; gets set to the accepted socket
 * @param errc out parameter; stores the operation's completion status
 * 
 * @return `EIO_OK` The accept has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A handler is already scheduled for the specified
 * acceptor
 * 
 * Reported status through `errc` may be:
 * 
 * `EIO_OK` The accept has completed successfully
 * 
 * `EIO_INVARG` The file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * acceptor
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 * 
 * `EIO_SYSERR` The accept failed. Cause is found via inspecting `errno`
 */
io_errcode async_accept(io_service *iosvc, int listen_sock,
                        struct sockaddr *addr, socklen_t *addrlen,
                        io_handler hnd, int *new_sock, io_errcode *errc);

/**
 * @brief Initiates an asynchronous connection on the provided service
 * 
 * @param iosvc service to schedule connection on
 * @param sockfd socket to peer
 * @param addr pointer to address of peer
 * @param addrlen pointer to size of `addr`
 * @param hnd completion handler
 * @param errc out parameter; stores the operation's completion status
 * 
 * @return `EIO_OK` The connection has been successfully scheduled
 * 
 * @return `EIO_NOMEM` Could not allocate necessary memory
 * 
 * @return `EIO_INVARG` The supplied handler's callback function is `NULL`
 * 
 * @return `EIO_STOPPED` The service has received a stop request
 * 
 * @return `EIO_INPROGRESS` A connect is already scheduled for provided `sockfd`
 * 
 * @return `EIO_SYSERR` The handshake can not be initiated. Inspect `errno`
 * 
 * Reported status through `errc` may be:
 * 
 * `EIO_OK` The connection has completed successfully
 * 
 * `EIO_INVARG` The file descriptor is invalid
 * 
 * `EIO_CANCELLED` A call to `iosvc_cancel()` was performed for the provided
 * connection
 * 
 * `EIO_STOPPED` A call to `iosvc_stop()` was made
 * 
 * `EIO_SYSERR` The connect failed. Cause is found via querying `getsockopt`
 * for the option `SO_ERROR` at level `SOL_SOCKET`
 */
io_errcode async_connect(io_service *iosvc, int sockfd,
                         struct sockaddr const *addr, socklen_t addrlen,
                         io_handler hnd, io_errcode *errc);

#endif // ASYNC_NET_H_
