#ifndef SENDER_H
#define SENDER_H

#include "types.h"

/*
 * Used to send data to a remote receiver. This is abstracted to support
 * raw calls to send(), for HTTP, and calls through SSL_send(), for HTTPS.
 */

typedef ssize_t (*sender_fn_t)(const char *const buf, const size_t len, const void *const arg);

int send_file(const char *const filepath, sender_fn_t sendf, const void *const arg);

#endif // SEND_H
