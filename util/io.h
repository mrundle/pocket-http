#ifndef IO_H
#define IO_H

#include "types.h"

/*
 * Used to send data to a remote receiver. This is abstracted to support
 * raw calls to send(), for HTTP, and calls through SSL_send(), for HTTPS.
 */

typedef ssize_t (*io_send_fn_t)(const char *const buf, const size_t len, const void *const arg);
typedef ssize_t (*io_recv_fn_t)(char *const buf, const size_t len, const void *const arg);

int io_send_file(const char *const filepath, io_send_fn_t sendf, const void *const arg);

// TODO io_read_http_header
// TODO io_send_response

#endif // SEND_H
