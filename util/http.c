#include "http.h"

#include "file.h"
#include "log.h"
#include "send.h"
#include "types.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static ssize_t
send_fn(const char *const buf, const size_t len, const void *const arg)
{
	if (buf == NULL || arg == NULL) {
		return -1;
	}
	int socket_desc = *((int *)arg);
	return send(socket_desc, buf, len, 0);
}

/*
 * TODO implement buffered reader
 */
int http_handle(const int socket_desc)
{
	log_info("received request");

	/* read line */
	char buf[1028];
	char *nwln = NULL;
	const ssize_t nread = recv(socket_desc, buf, sizeof(buf), 0);
	if (nread <= 0 || (nwln = strchr(buf, '\n')) == NULL) {
		log_error("no info read, or no newline found");
		return -1;
	}
	const unsigned len = (unsigned)(nwln - buf);
	char buf2[1028];
	strncpy(buf2, buf, len);
	buf2[len] = '\0';
	log_info("got line: %s", buf2);

	buf[sizeof(buf)-1] = '\0';
	log_info("got all: '%s'", buf);


	/* write the file */
	const char *const file = "docroot/index.html";
	ssize_t file_size;
	if (get_file_size(file, &file_size) != 0) {
		log_error("failed to get size of %s", file);
		return -1;
	}

	char resp[128];	
	snprintf(resp, sizeof(resp), "HTTP/1.0 200 OK\r\n\r\n");
	send(socket_desc, resp, strlen(resp), 0);
#if 0
	snprintf(resp, sizeof(resp), "HTTP/1.0 200 OK\r\n");
	send(socket_desc, resp, strlen(resp), 0);

	snprintf(resp, sizeof(resp), "Content-Type: text/html\r\n");
	send(socket_desc, resp, strlen(resp), 0);

	snprintf(resp, sizeof(resp), "Content-Length: %lu\r\n\r\n", file_size);
	send(socket_desc, resp, strlen(resp), 0);
#endif

	log_info("size of %s is %ld", file, file_size);
	if (send_file(file, send_fn, &socket_desc) != 0) {
		log_error("failed to serve file");
		return -1;
	}

	return 0;
}
