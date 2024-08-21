#ifndef IOSVC_DEQUEUE_H_
#define IOSVC_DEQUEUE_H_ 1

// Used in iosvc_run and iosvc_cancel, when dequeuing async events

#include "iosvc_def.h"
#include "rbtree.h"

/**
 * @brief Extract a handler from the service and unset its event, vacating
 * the space for a subsequent one
 * 
 * @param iosvc service to remove the handler from
 * @param fd_node node to remove the handler from
 * @param event_type event wait type
 * @param now current time
 * @param status status to signal to the callback
 * @return Extracted `io_handler`
 */
io_handler iosvc_dequeue(io_service *iosvc, rb_node *fd_node,
                         io_wait_type event_type, int64_t now,
                         io_errcode status);

/**
 * @brief Remove an event group node (each node contains all possible event
 * types for a single FD) from the events set and its associated
 * pollfd.
 * 
 * Removal of pollfd is done by moving the last pollfd in the poll vector to
 * the position held by the provided node.
 * 
 * Typically called when the associated pollfd's events is zero.
 * 
 * @param iosvc service to remove the event group from
 * @param place place of node in set (tree)
 * @param parent parent of node
 */
void ioevt_cleanup_evt(io_service *iosvc, rb_place place, rb_node *parent);

#endif // IOSVC_DEQUEUE_H_
