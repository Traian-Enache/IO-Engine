#ifndef IO_SEMAPHORE_H_
#define IO_SEMAPHORE_H_

#include "io_service.h"

typedef struct _io_sem_t io_semaphore;

io_semaphore *iosem_create(int init_val, io_service *iosvc);

void iosem_delete(io_semaphore *iosem);

io_errcode iosem_wait(io_semaphore *iosem, io_handler hnd);

io_errcode iosem_signal(io_semaphore *iosem);

#endif // IO_SEMAPHORE_H_
