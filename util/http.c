#include "http.h"

#include "file.h"
#include "io.h"
#include "log.h"
#include "types.h"
#include "request.h" // the real http.h TODO rename

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static ssize_t
http_send_fn(const char *const buf, const size_t len, const void *const arg)
{
	if (buf == NULL || arg == NULL) {
		return -1;
	}
	int socket_desc = *((int *)arg);
	return send(socket_desc, buf, len, 0);
}

static ssize_t
http_recv_fn(char *const buf, const size_t len, const void *const arg)
{
	if (buf == NULL || arg == NULL) {
		return -1;
	}
	int socket_desc = *((int *)arg);
	return recv(socket_desc, buf, len, 0);
}

/*
 * TODO implement buffered reader. Requests are ended by `\r\n\r\n`,
 * so look for that.
 */
int http_handle(const int socket_desc)
{
	return handle_http_request(http_send_fn, http_recv_fn, &socket_desc);
}
