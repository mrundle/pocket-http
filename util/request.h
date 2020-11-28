#ifndef REQUEST_H
#define REQUEST_H

#include "io.h"

int handle_http_request(
	io_send_fn_t send_fn,
	io_recv_fn_t recv_fn,
	const void *const io_arg);

#endif // REQUEST_H
