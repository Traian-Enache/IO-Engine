#include "iotypes.h"

const char *ioec_strerr(io_errcode errc) {
    static char const * const errc_names[] = {
        [EIO_OK] = "EIO_OK",
        [EIO_TIMEOUT] = "EIO_TIMEOUT",
        [EIO_CANCELLED] = "EIO_CANCELLED",
        [EIO_STOPPED] = "EIO_STOPPED",
        [EIO_INVARG] = "EIO_INVARG",
        [EIO_NOENTRY] = "EIO_NOENTRY",
        [EIO_NOMEM] = "EIO_NOMEM",
        [EIO_INPROGRESS] = "EIO_INPROGRESS",
    };

    return errc_names[errc];
}
